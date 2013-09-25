//#include <config.h>
#include <Python.h>
#include <rec.h>
#include "structmember.h"
#include <error.h>

typedef struct {
    PyObject_HEAD   
    rec_db_t rdb;  
} recdb;


typedef struct {
    PyObject_HEAD
    rec_rset_t rst;  
} rset;

typedef struct {
    PyObject_HEAD 
    rec_record_t rcd;  
} record;

typedef struct {
    PyObject_HEAD
    rec_field_t fld;  
} field;

typedef struct {
    PyObject_HEAD
    rec_sex_t sx;  
} sex;

typedef struct {
    PyObject_HEAD
    rec_fex_t fx;  
} fex;

typedef struct {
    PyObject_HEAD
    rec_fex_elem_t fxel;  
} fexelem;


typedef struct {
    PyObject_HEAD
    rec_comment_t cmnt;  
} comment;

typedef struct {
    PyObject_HEAD
    rec_buf_t buf;  
} buffer;

staticforward PyTypeObject rsetType;
staticforward PyTypeObject recordType;
staticforward PyTypeObject fexType;
staticforward PyTypeObject fexelemType;
staticforward PyTypeObject sexType;
staticforward PyTypeObject fieldType;
staticforward PyTypeObject commentType;
staticforward PyTypeObject bufferType;
static PyObject *RecError;

/* Create an empty database.  */

static PyObject *
recdb_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  recdb *self;
  self = (recdb *) type->tp_alloc (type, 0);
  //Py_INCREF(self);
  if (self != NULL) 
    {
        self->rdb = rec_db_new();
        
        if (self->rdb == NULL) 
          {
             /* Out of memory.  */
            Py_DECREF(self);
            return NULL;
          }
    }
  return (PyObject *)self;
}


/* Destroy a database, freeing any used memory.
 *
 * This means that all the record sets contained in the database are
 * also destroyed.
 */

static void
recdb_dealloc (recdb* self)
{
  self->ob_type->tp_free ((PyObject*) self);
}


/* Return the number of record sets contained in a given database  */
static PyObject*
recdb_size (recdb* self)
{
  int s = rec_db_size (self->rdb);
  return Py_BuildValue ("i", s);
}


/* Load a file into a Database object */

static PyObject*
recdb_pyloadfile (recdb *self, PyObject *args, PyObject *kwds)
{
  char *string = NULL;
  bool success;
  rec_parser_t parser;
  static char *kwlist[] = {"filename", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &string)) 
    {
      return NULL;
    }
  FILE *in = fopen (string,"r");
  if (in == NULL)
    {
      PyErr_SetString (RecError, strerror (errno));
      return NULL;
    }
  parser = rec_parser_new (in, string);
  rec_db_destroy (self->rdb);
  success = rec_parse_db (parser, &(self->rdb));
  if (!success)
    {
      PyErr_SetString (RecError, "parse error");
      return NULL;
    }
  fclose (in);
  rec_parser_destroy (parser);
  return Py_BuildValue ("");
}

/* Append a file into a Database object */

static PyObject*
recdb_pyappendfile (recdb *self, PyObject *args, PyObject *kwds)
{
  char *string = NULL;
  bool success = true;
  char str[100];
  rec_rset_t res;  
  rec_parser_t parser;
  static char *kwlist[] = {"filename", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "s", kwlist, &string)) 
    {
      return NULL;
    }
  FILE *in = fopen (string,"r");
  if (in == NULL)
    {
      PyErr_SetString (RecError, strerror (errno));
      return NULL;
    }
  parser = rec_parser_new (in, string);
  while (rec_parse_rset (parser, &res))
    {
      char *rset_type;
      /* XXX: check for consistency!!!.  */
      rset_type = rec_rset_type (res);
      if (rec_db_type_p (self->rdb, rset_type))
        {
          sprintf (str,"Duplicated record set '%s' from %s.",rset_type, string);
          PyErr_SetString (RecError, str);
          return NULL;
        }

      if (!rec_db_insert_rset (self->rdb, res, rec_db_size (self->rdb)))
        {
          /* Error.  */
          success = false;
          break;
        }
    }

  if (rec_parser_error (parser))
    {
      /* Report parsing errors.  */
      rec_parser_perror (parser, "%s", string);
      success = false;
    }
  rec_parser_destroy (parser);
  fclose (in);
  if (!success)
    {
      PyErr_SetString (RecError, "parse error");
      return NULL;
    }
  return Py_BuildValue ("");
}

/*Write to file from a DB object */ 

static PyObject*
recdb_pywritefile (recdb *self, PyObject *args, PyObject *kwds)
{
  char *string = NULL;
  bool success;
  static char *kwlist[] = {"filename", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "s", kwlist, &string))
    {
      return NULL;
    }
  FILE *out = fopen (string,"w");
  if (out == NULL)
    {
      PyErr_SetString (RecError, strerror (errno));
      return NULL;
    }
  rec_writer_t writer = rec_writer_new (out);
  success = rec_write_db (writer, self->rdb);
  if (!success)
    {
      PyErr_SetString (RecError, "parse error");
      return NULL;
    }
  fclose (out);
  return Py_BuildValue ("");

}


/* Return the record set occupying the given position in the database.
   If no such record set is contained in the database then None is
   returned.  */

static PyObject*
recdb_get_rset (recdb *self, PyObject *args, PyObject *kwds)
{
  int pos;
  rec_rset_t res;
  PyObject *result;
  rset *tmp = PyObject_NEW (rset, &rsetType);
  //tmp->rst = rec_rset_new();
  static char *kwlist[] = {"position", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &pos)) 
    {
      return NULL;
    }
  res = rec_db_get_rset (self->rdb, pos);
  tmp->rst = res;
  result =  Py_BuildValue ("O",tmp);
  return result;
}

/* Insert the given record set into the given database at the given
 * position.
 *
 * - If POSITION >= rec_rset_size (DB), RSET is appended to the
 *   list of fields.
 *
 * - If POSITION < 0, RSET is prepended.
 *
 * - Otherwise RSET is inserted at the specified position.
 *
 * If the rset is inserted then 'true' is returned. If there is an
 * error then 'false' is returned.
 */

static PyObject*
recdb_pyinsert_rset (recdb *self, PyObject *args, PyObject *kwds)
{
  rset *recset;
  size_t position;
  bool success;
  static char *kwlist[] = {"recset", "position",NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "Oi", kwlist, 
                                    &recset,
                                    &position))
    {
      return NULL; 
    }
  success = rec_db_insert_rset (self->rdb, recset->rst, position);
  if (!success)
    {
      PyErr_SetString (RecError, "Record set insertion failed");
      return NULL;
    }
  return Py_BuildValue ("");
}


/* Remove the record set contained in the given position into the
   given database.  If POSITION >= rec_db_size (DB), the last record
   set is deleted.  If POSITION <= 0, the first record set is deleted.
   Otherwise the record set occupying the specified position is
   deleted.  If a record set has been removed then 'true' is returned.
   If there is an error or the database has no record sets 'false' is
   returned.  */

static PyObject*
recdb_pyremove_rset (recdb *self, PyObject *args, PyObject *kwds)
{
  size_t position;
  bool success;
  static char *kwlist[] = {"position",NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "i", kwlist, 
                                    &position))
    {
      return NULL;
    }
  success = rec_db_remove_rset (self->rdb, position);
  if (!success)
    {
      PyErr_SetString (RecError, "Record set deletion failed");
      return NULL;
    }
  return Py_BuildValue ("");
}

/* Determine whether an rset named TYPE exists in a database.  If TYPE
   is NULL then it refers to the default record set.  */

static PyObject*
recdb_type (recdb *self, PyObject *args)
{
    char *type;
    if (!PyArg_ParseTuple (args, "s", &type)) 
      {
        return NULL;
      }
    bool success = rec_db_type_p (self->rdb,type);
    return Py_BuildValue ("i",success);

}


/* Get the rset with the given type from db.  This function returns
NULL if there is no a record set having that type.  */

static PyObject*
recdb_get_rset_by_type (recdb *self, PyObject *args, PyObject *kwds)
{
    char *type = NULL;
    rec_rset_t res;
    PyObject *result;
    rset *tmp = PyObject_NEW(rset, &rsetType);
    static char *kwlist[] = {"type", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s", kwlist, &type)) 
      {
        return NULL;
      }
    res = rec_db_get_rset_by_type (self->rdb,type);
    tmp->rst = res;
    result =  Py_BuildValue ("O",tmp);
    return result;
}

/******************** Database High-Level functions *******************/

/* Query for some data in a database.  The resulting data is returned
   in a record set. 

   This function takes the following arguments:

   DB

      Database to query.

   TYPE 

      The type of records to query.  This string must identify a
      record set contained in the database.  If TYPE is NULL then the
      default record set, if any, is queried.

   JOIN
   
      If not NULL, this argument must be a string denoting a field
      name.  This field name must be a foreign key (field of type
      'rec') defined in the selected record set.  The query operation
      will do an inner join using T1.Field = T2.Field as join
      criteria.

   INDEX

      If not NULL, this argument is a pointer to a buffer containing
      pairs of Min,Max indexes, identifying intervals of valid
      records.  The list of ends with the pair
      REC_Q_NOINDEX,REC_Q_NOINDEX.

      INDEX is mutually exclusive with any other selection option.

   SEX

      Selection expression which is evaluated for every record in the
      referred record set.  If SEX is NULL then all records are
      selected.

      This argument is mutually exclusive with any other selection
      option.

   FAST_STRING

      If this argument is not NULL then it is a string which is used
      as a fixed pattern.  Records featuring fields containing
      FAST_STRING as a substring in their values are selected.

      This argument is mutually exclusive with any other selection
      option.

   RANDOM

      If not 0, this argument indicates the number of random records
      to select from the referred record set.
 
      This argument is mutually exclusive with any other selection
      option.

   FEX

      Field expression to apply to the matching records to build the
      records in the result record set.  If FEX is NULL then the
      matching records are unaltered.

   PASSWORD

      Password to use to decrypt confidential fields.  If the password
      does not work then the encrypted fields are returned as-is.  If
      PASSWORD is NULL, or if it is the empty string, then no attempt
      to decrypt encrypted fields will be performed.

   GROUP_BY

      If not NULL, group the record set by the given field names.
 
   SORT_BY

      If not NULL, sort the record set by the given field names.

   FLAGS

      ORed value of any of the following flags:

      REC_Q_DESCRIPTOR

      If set returned record set will feature a record descriptor.  If
      the query is involving a single record set then the descriptor
      will be a copy of the descriptor of the referred record set, and
      will feature the same record type name.  Otherwise it will be
      built from the several descriptors of the involved record sets,
      and the record type name will be formed concatenating the type
      names of the involved record sets.  If this flag is not
      activated then the returned record set won't feature a record
      descriptor.

      REC_Q_ICASE

      If set the string operations in the selection expression will be
      case-insensitive.  If FALSE any string operation will be
      case-sensitive.

  This function returns NULL if there is not enough memory to
  perform the operation.  */

static PyObject*
recdb_query (recdb *self, PyObject *args, PyObject *kwds)
{
  const char  *type;
  const char  *join;
  size_t      *index;
  sex         *sexp;
  const char  *fast_string;
  size_t       random;
  fex         *fexp;
  const char  *password;
  fex         *group_by;
  fex         *sort_by;
  int          flags;
  rset        *tmp = PyObject_NEW (rset, &rsetType); //return type
  rec_rset_t res;
  static char *kwlist[] = {"type", "join", "index", "sexp",
                           "fast_string", "random", "fexp",
                           "password", "group_by", "sort_by",
                           "flags", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zzzOziOzOOi", kwlist,
                                    &type, &join, &index, &sexp, &fast_string, 
                                    &random, &fexp, &password, &group_by, 
                                    &sort_by, &flags))
    { 

      return NULL;
    }
  if ((PyObject *)sexp == Py_None)
      sexp->sx = NULL;
  if ((PyObject *)fexp == Py_None)
      fexp->fx = NULL;
  if ((PyObject *)group_by == Py_None)
      group_by->fx = NULL;
  if ((PyObject *)sort_by == Py_None)
      sort_by->fx = NULL;
  res = rec_db_query (self->rdb, type, join, index,
                      sexp->sx, fast_string, random,
                      fexp->fx, password, group_by->fx,
                      sort_by->fx, flags);
  tmp->rst = res;
  return Py_BuildValue ("O",tmp);
}


/* Insert a new record into a database, either appending it to some
   record set or replacing one or more existing records.

   This function takes the following arguments:
   
   DB

      Database where to insert the record.

   TYPE

      Type of the new record.  If there is an existing record set
      holding records of that type then the record is added to it.
      Otherwise a new record set is appended into the database.

   INDEX

      If not NULL, this argument is a pointer to a buffer containing
      pairs of Min,Max indexes, identifying intervals of records that
      will be replaced by copies of the provided record. The list of
      ends with the pair REC_Q_NOINDEX,REC_Q_NOINDEX.

      INDEX is mutually exclusive with any other selection option.

   SEX

      Selection expression which is evaluated for every record in the
      referred record set.  If SEX is NULL then all records are
      selected.

      This argument is mutually exclusive with any other selection
      option.

   FAST_STRING

      If this argument is not NULL then it is a string which is used
      as a fixed pattern.  Records featuring fields containing
      FAST_STRING as a substring in their values are selected.

      This argument is mutually exclusive with any other selection
      option.

   RANDOM

      If not 0, this argument indicates the number of random records
      to select from the referred record set.
 
      This argument is mutually exclusive with any other selection
      option.

   PASSWORD

      Password to use to crypt confidential fields.  If PASSWORD is
      NULL, or if it is the empty string, then no attempt to crypt
      confidential fields will be performed.

   RECORD

      Record to insert.  If more than one record is replaced in the
      database they will be substitued with copies of this record.

   FLAGS

      ORed value of any of the following flags:

      REC_F_ICASE

      If set the string operations in the selection expression will be
      case-insensitive.  If FALSE any string operation will be
      case-sensitive.

      REC_F_NOAUTO

      If set then no auto-fields will be added to the newly created
      records in the database.

   If no selection option is used then the new record is appended to
   either an existing record set identified by TYPE or to a newly
   created record set.  If some selection option is used then the
   matching existing records will be replaced.

   This function returns 'false' if there is not enough memory to
   perform the operation.  */


static PyObject*
recdb_insert (recdb *self, PyObject *args, PyObject *kwds)
{

  const char  *type;
  size_t      *index;
  sex         *sexp;
  const char  *fast_string;
  size_t       random;
  const char  *password;
  record      *recp;
  int          flags;
  bool success; 
  static char *kwlist[] = {"type", "index", "sexp",
                           "fast_string", "random",
                           "password", "recp",
                           "flags", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zzOzizOi", kwlist,
                                    &type, &index, &sexp, &fast_string, 
                                    &random, &password, &recp, &flags))

    { 

      return NULL;
    }
  if ((PyObject *)sexp == Py_None)
      sexp->sx = NULL;
  success = rec_db_insert (self->rdb, type, index,
                           sexp->sx, fast_string, random,
                           password, recp->rcd, flags);
  return Py_BuildValue ("i",success);
}


/* Delete records from a database, either physically removing them or
   commenting them out.

   This function takes the following arguments:

   DB

      Database where to remove records.

   TYPE

      Type of the records to remove.

   INDEX

      If not NULL, this argument is a pointer to a buffer containing
      pairs of Min,Max indexes, identifying intervals of records that
      will be deleted or commented out. The list of ends with the pair
      REC_Q_NOINDEX,REC_Q_NOINDEX.

      INDEX is mutually exclusive with any other selection option.

   SEX

      Selection expression which is evaluated for every record in the
      referred record set.  If SEX is NULL then all records are
      selected.

      This argument is mutually exclusive with any other selection
      option.

   FAST_STRING

      If this argument is not NULL then it is a string which is used
      as a fixed pattern.  Records featuring fields containing
      FAST_STRING as a substring in their values are selected.

      This argument is mutually exclusive with any other selection
      option.
 
   RANDOM

      If not 0, this argument indicates the number of random records
      to select for deletion in the referred record set.
 
      This argument is mutually exclusive with any other selection
      option.

   FLAGS

      ORed value of any of the following flags:

      REC_F_ICASE

      If set the string operations in the selection expression will be
      case-insensitive.  If FALSE any string operation will be
      case-sensitive.

      REC_F_COMMENT_OUT

      If set the selected records will be commented out instead of physically
      removed from the database.

  This function returns 'false' if there is not enough memory to
  perform the operation.  */

static PyObject*
recdb_delete (recdb *self, PyObject *args, PyObject *kwds)
{

  const char  *type;
  size_t      *index;
  sex         *sexp;
  const char  *fast_string;
  size_t       random;
  int          flags;
  bool success; 
  static char *kwlist[] = {"type", "index", "sexp",
                           "fast_string", "random",
                           "flags", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zzOzii", kwlist,
                                    &type, &index, &sexp, &fast_string, 
                                    &random, &flags))

    { 

      return NULL;
    }
  if ((PyObject *)sexp == Py_None)
      sexp->sx = NULL;
  success = rec_db_delete (self->rdb, type, index,
                           sexp->sx, fast_string, 
                           random, flags);
  return Py_BuildValue ("i",success);
}


/* Manipulate the fields of the selected records in a database: remove
   them, set their values or rename them.

   This function takes the following arguments:

   DB

      Database where to set fields.

   TYPE

      Type of the records to act in.

   INDEX

      If not NULL, this argument is a pointer to a buffer containing
      pairs of Min,Max indexes, identifying intervals of records that
      will be deleted or commented out. The list of ends with the pair
      REC_Q_NOINDEX,REC_Q_NOINDEX.

      INDEX is mutually exclusive with any other selection option.

   SEX

      Selection expression which is evaluated for every record in the
      referred record set.  If SEX is NULL then all records are
      selected.

      This argument is mutually exclusive with any other selection
      option.

   FAST_STRING

      If this argument is not NULL then it is a string which is used
      as a fixed pattern.  Records featuring fields containing
      FAST_STRING as a substring in their values are selected.

      This argument is mutually exclusive with any other selection
      option.
 
   RANDOM

      If not 0, this argument indicates the number of random records
      to select for manipulation in the referred record set.
 
      This argument is mutually exclusive with any other selection
      option.
   
   FEX

      Field expression selecting the fields in the selected records
      which will be modified.

   ACTION

      Action to perform to the selected fields.  Valid values for this
      argument are:

      REC_SET_ACT_RENAME

      Rename the matching fields to the string pointed by ACTION_ARG.

      REC_SET_ACT_SET

      Set the value of the matching fields to the string pointed by
      ACTION_ARG.

      REC_SET_ACT_ADD

      Add new fields with the names specified in the fex to the
      selected records.  The new fields will have the string pointed
      by ACTION_ARG as their value.

      REC_SET_ACT_SETADD

      Set the selected fields to the value pointed by ACTION_ARG.  IF
      the fields dont exist then create them with that value.

      REC_SET_ACT_DELETE

      Delete the selected fields.  ACTION_ARG is ignored by this
      action.

      REC_SET_ACT_COMMENT

      Comment out the selected fields.  ACTION_ARG is ignored by this
      action.
      
   ACTION_ARG

      Argument to the selected action.  It is ok to pass NULL for
      actions which dont require an argument.

   FLAGS

      ORed value of any of the following flags:

      REC_F_ICASE

      If set the string operations in the selection expression will be
      case-insensitive.  If FALSE any string operation will be
      case-sensitive.

   This function returns'false' if there is not enough memory to
   perform the operation.
*/

static PyObject*
recdb_set (recdb *self, PyObject *args, PyObject *kwds)
{

  const char  *type;
  size_t      *index;
  sex         *sexp;
  const char  *fast_string;
  size_t       random;
  fex         *fexp;
  int          action;
  const char   *action_arg;
  int          flags;
  bool success; 
  static char *kwlist[] = {"type", "index", "sexp",
                           "fast_string", "random", 
                           "fexp", "action", "action_arg",
                           "flags", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zzOziOizi", kwlist,
                                    &type, &index, &sexp, &fast_string, 
                                    &random, &fexp, &action, &action_arg, &flags))

    { 

      return NULL;
    }
  if ((PyObject *)sexp == Py_None)
      sexp->sx = NULL;
  if ((PyObject *)fexp == Py_None)
      fexp->fx = NULL;
  success = rec_db_set (self->rdb, type, index,
                        sexp->sx, fast_string, random,
                        fexp->fx, action, action_arg, flags);
  return Py_BuildValue ("i",success);
}

/* Check the integrity of all the record sets stored in a given
   database.  This function returns the number of errors found.
   Descriptive messages about the errors are appended to ERRORS.  */

static PyObject*
recdb_int_check (recdb *self, PyObject *args, PyObject *kwds)
{

  bool check_descriptors_p;
  bool remote_descriptors_p;
  buffer *errors;
  int num;
  static char *kwlist[] = {"check_descriptors_p","remote_descriptors_p","errors", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "iiO", kwlist,
                                    &check_descriptors_p, &remote_descriptors_p, &errors))

    { 

      return NULL;
    }

   num = rec_int_check_db (self->rdb, check_descriptors_p, remote_descriptors_p, errors->buf);
   return Py_BuildValue ("i", num);
 }

/*recdb doc string */
static char recdb_doc[] =
  "This type refers to the database structure of recutils";


static PyMethodDef recdb_methods[] = {
    {"size", (PyCFunction)recdb_size, METH_NOARGS,
     "Return the size of the DB"
    },
    {"pyloadfile", (PyCFunction)recdb_pyloadfile, 
     METH_VARARGS, 
     "Load data from file into DB"
    },
    {"pywritefile", (PyCFunction)recdb_pywritefile, 
     METH_VARARGS, 
     "Write data from DB to file"
    },
    {"pyappendfile", (PyCFunction)recdb_pyappendfile, 
     METH_VARARGS, 
     "Append data from DB to file"
    },
    {"get_rset", (PyCFunction)recdb_get_rset, 
     METH_VARARGS, 
     "Get rset by position"
    },
    {"pyinsert_rset", (PyCFunction)recdb_pyinsert_rset, 
     METH_VARARGS, 
     "Insert rset into DB"
    },
    {"pyremove_rset", (PyCFunction)recdb_pyremove_rset, 
     METH_VARARGS, 
     "Remove rset from DB"
    },
    {"type", (PyCFunction)recdb_type, 
     METH_VARARGS, 
     "Determine if the rset of type TYPE exists"
    },
     {"get_rset_by_type", (PyCFunction)recdb_get_rset_by_type, 
     METH_VARARGS, 
     "Get rset by type"
    },
    {"query", (PyCFunction)recdb_query, 
     METH_VARARGS, 
     "Query the DB"
    },
    {"insert", (PyCFunction)recdb_insert, 
     METH_VARARGS, 
     "Insert a record into DB"
    },
    {"delete", (PyCFunction)recdb_delete, 
     METH_VARARGS, 
     "Delete a record from DB"
    },
    {"set", (PyCFunction)recdb_set, 
     METH_VARARGS, 
     "Manipulate a record in DB"
    },
    {"int_check", (PyCFunction)recdb_int_check, 
     METH_VARARGS, 
     "Check the integrity of all the record sets stored in DB"
    },

    {NULL}  
};

/* Define the recdb object type */
static PyTypeObject recdbType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.recdb",              /*tp_name*/
    sizeof(recdb),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)recdb_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    recdb_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    recdb_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    recdb_new,                 /* tp_new */
};




/* Create a new empty record set and return a reference to it.  NULL
   is returned if there is no enough memory to perform the
   operation.  */

static PyObject *
rset_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  rset *self;
  self = (rset *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
        self->rst = rec_rset_new();
        if (self->rst == NULL) 
          {
              /*Out of memory*/
            Py_DECREF (self);
            return NULL;
          }
    }
  return (PyObject *)self;
}


/* Destroy a record set, freeing all user resources.  This disposes
   all the memory used by the record internals, including any stored
   record or comment.  */

static void
rset_dealloc (rset* self)
{
  rec_rset_destroy (self->rst);
  self->ob_type->tp_free ((PyObject*)self);
}


/* Return the number of records stored in the given record set.  */

static PyObject*
rset_num_records (rset* self)
{
  int num = rec_rset_num_records (self->rst);
  return Py_BuildValue ("i",num);
}

/* Return the record descriptor of a given record set.  NULL is
   returned if the record set does not feature a record
   descriptor.  */

static PyObject*
rset_descriptor (rset* self)
{
  PyObject *result;
  rec_record_t reco;
  record *tmp = PyObject_NEW (record, &recordType);
  reco = rec_rset_descriptor (self->rst);
  //printf("The desc is: %s", reco->source);
  tmp->rcd = reco;
  result = Py_BuildValue ("O",tmp);
  return result;
}

/* Return the type name of a record set.  NULL is returned if the
   record set does not feature a record descriptor.  */

static PyObject*
rset_type (rset* self)
{
  PyObject *result;
  char* restype;
  restype = rec_rset_type (self->rst);
  result = Py_BuildValue ("s",restype);
  return result;
}

/*rset doc string */
static char rset_doc[] =
  "This type refers to the record set structure of recutils";


static PyMethodDef rset_methods[] = {
    {"num_records", (PyCFunction)rset_num_records, METH_NOARGS,
     "Return the number of records in the record set"  
    },
    {"descriptor", (PyCFunction)rset_descriptor, METH_NOARGS,
     "Return the descriptor of the record set"  
    },
    {"type", (PyCFunction)rset_type, METH_NOARGS,
     "Return the type name of a record set"
   },
    {NULL}
};


/* Define the rset object type */
static PyTypeObject rsetType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.rset",              /*tp_name*/
    sizeof(rset),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)rset_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    rset_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    rset_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    rset_new,                 /* tp_new */
};


/* Create a new empty record and return a reference to it.  NULL is
   returned if there is no enough memory to perform the operation.  */
static PyObject *
record_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  record *self;
  self = (record *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
        self->rcd = rec_record_new();
        if (self->rcd == NULL) 
          {
             /* Out of memory.  */
            Py_DECREF (self);
            return NULL;
          }
    }
  return (PyObject *)self;
}

/* Destroy a record, freeing all used resources.  This disposes all
   the memory used by the record internals, including any stored field
   or comment.  */

static void
record_dealloc (record* self)
{
  rec_record_destroy (self->rcd);
  self->ob_type->tp_free ((PyObject*)self);
}  

/* Return the number of fields stored in the given record.  */

static PyObject*
record_num_fields (record* self)
{
  int num = rec_record_num_fields (self->rcd);
  return Py_BuildValue ("i",num);

}

/* Determine whether a record contains some field whose value is VALUE.
   The string comparison can be either case-sensitive or
   case-insensitive.  */

static PyObject*
record_contains_value (record* self, PyObject *args, PyObject *kwds)
{
  const char *value = NULL;
  bool case_insensitive;
  bool success;
  static char *kwlist[] = {"value", "case_insensitive", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "si", kwlist, &value, &case_insensitive)) 
    {
      return NULL;
    }
  success = rec_record_contains_value (self->rcd, value, case_insensitive);
  return Py_BuildValue ("i",success);

}

/* Determine whether a record contains a field whose name is
   FIELD_NAME and value FIELD_VALUE.  */


static PyObject*
record_contains_field (record* self, PyObject *args, PyObject *kwds)
{
  const char *field_name = NULL;
  const char *field_value = NULL;
  static char *kwlist[] = {"field_name", "field_value", NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "ss", kwlist, &field_name, &field_value)) 
    {
      return NULL;
    }
  bool success = rec_record_contains_field (self->rcd, field_name, field_value);
  return Py_BuildValue ("i",success);

}



/*record doc string */
static char record_doc[] =
  "This type refers to the record structure of recutils";


static PyMethodDef record_methods[] = {
    {"num_fields", (PyCFunction)record_num_fields, METH_NOARGS,
     "Return the number of fields in the record"  
    },
    {"contains_value", (PyCFunction)record_contains_value, METH_VARARGS,
     "Determine whether a record contains some field whose value is STR"  
    },
    {"contains_field", (PyCFunction)record_contains_field, METH_VARARGS,
     "Determine whether a record contains a field whose name is FIELD_NAME and value FIELD_VALUE."  
    },
    {NULL}
};




/* Define the record object type */
static PyTypeObject recordType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.record",              /*tp_name*/
    sizeof(record),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)record_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    record_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    record_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    record_new,                 /* tp_new */
};


/* Create a new selection expression and return it.  If there is not
   enough memory to create the sex, then return NULL.  */
static PyObject *
sex_new (PyTypeObject *type, PyObject *args, PyObject *kwds) 
{
  bool case_insensitive;
  sex *self;
  static char *kwlist[] = {"case_insensitive",NULL};
  if (!PyArg_ParseTupleAndKeywords(args, kwds, "i", kwlist, &case_insensitive)) 
    {
      return NULL;
    }
  self = (sex *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
      self->sx = rec_sex_new(case_insensitive);
      
      if (self->sx == NULL) 
        {
           /* Out of memory.  */
          Py_DECREF (self);
          return NULL;
        }
    }
  return (PyObject *)self;
}

/* Destroy a sex, freeing any used resources.  */
static void
sex_dealloc (sex* self)
{
  rec_sex_destroy (self->sx);
  self->ob_type->tp_free ((PyObject*)self);
}

/* Compile a sex.  Sexes must be compiled before being used.  If there
   is a parse error return false.  */

static PyObject*
sex_pycompile (sex *self, PyObject *args, PyObject *kwds)
{
  const char *expr;
  static char *kwlist[] = {"expr",NULL};
  bool success;
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "z", kwlist, &expr)) 
    {
      return NULL;
    }
  success = rec_sex_compile (self->sx,expr);
  return Py_BuildValue ("i",success);
}

/* Apply a sex expression to a record, setting STATUS in accordance:
   'true' if the record matched the sex, 'false' otherwise.  The
   function returns the same value that is stored in STATUS.  */

static PyObject*
sex_pyeval (sex *self, PyObject *args, PyObject *kwds)
{
  bool status;
  record *rec;
  bool success;
  static char *kwlist[] = {"rec", "status",NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "Oi", kwlist, 
                                    &rec,
                                    &status))
    {
      return NULL; 
    }
  success = rec_sex_eval (self->sx, rec->rcd, &status);
  return Py_BuildValue ("i",success);
}

/* Apply a sex expression and get the result as an allocated
   string.  */

static PyObject*
sex_eval_str (sex *self, PyObject *args, PyObject *kwds)
{
  record *rec;
  char *str;  
  static char *kwlist[] = {"rec",NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "O", kwlist, 
                                    &rec))
    {
    return NULL; 
    }
  str = rec_sex_eval_str (self->sx, rec->rcd);   
  return Py_BuildValue ("z",str);
}


/*record doc string */
static char sex_doc[] =
  "This type refers to the selection expression structure of recutils";


static PyMethodDef sex_methods[] = {
    {"pycompile", (PyCFunction)sex_pycompile, METH_VARARGS,
     "Compile a sex. If there is a parse error return false."  
    },
    {"pyeval", (PyCFunction)sex_pyeval, METH_VARARGS,
     "Apply a sex expression to a record, setting STATUS to true if it matches and false otherwise."  
    },
    {"eval_str", (PyCFunction)sex_eval_str, METH_VARARGS,
     "Apply a sex expression and get the result as an allocated string."  
    },
    {NULL}
};


/* Define the record object type */
static PyTypeObject sexType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.sex",              /*tp_name*/
    sizeof(sex),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)sex_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    sex_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    sex_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    sex_new,                 /* tp_new */
};


/* Parse and create a field expression, and return it.  A fex kind
   shall be specified in KIND.  If STR does not contain a valid FEX of
   the given kind then NULL is returned.  If there is not enough
   memory to perform the operation then NULL is returned.  If STR is
   NULL then an empty fex is returned.  */

static PyObject *
fex_new (PyTypeObject *type, PyObject *args, PyObject *kwds) 
{
  fex *self;
  const char *str;
  enum rec_fex_kind_e kind;
  static char *kwlist[] = {"str","kind", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zi", kwlist, &str, &kind)) 
    {
      return NULL;
    }
  self = (fex *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
      self->fx = rec_fex_new (str,kind);
      if (self->fx == NULL) 
        {
           /* Out of memory.  */
          Py_DECREF (self);
          return NULL;
        }
    }
  return (PyObject *)self;
}

/* Destroy a field expression, freeing any used resource. */

static void
fex_dealloc (fex* self)
{
  rec_fex_destroy (self->fx);
  self->ob_type->tp_free ((PyObject*)self);
}


/* Get the number of elements stored in a field expression.  */

static PyObject*
fex_size (fex *self)
{
  int num;
  num = rec_fex_size (self->fx);
  return Py_BuildValue ("i", num);
}

/* Check whether a given field (or set of fields) identified by their
   name and indexes, are contained in a fex.  */

static PyObject*
fex_member_p (fex *self, PyObject *args, PyObject *kwds)
{
  const char *fname;
  int min, max;
  bool success;
  static char *kwlist[] = {"fname","min","max", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zii", kwlist, &fname, &min, &max)) 
    {
      return NULL;
    }
  success = rec_fex_member_p (self->fx, fname, min, max);
  return Py_BuildValue ("i", success);
}


/* Get the element of a field expression occupying the given position.
   If the position is invalid then NULL is returned.  */

static PyObject*
fex_get (fex *self, PyObject *args, PyObject *kwds)
{
  size_t position;
  fexelem *tmp = PyObject_NEW (fexelem, &fexelemType);
  rec_fex_elem_t fexel;
  static char *kwlist[] = {"position", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "i", kwlist, &position)) 
    {
      return NULL;
    }
  fexel = rec_fex_get (self->fx, position);
  tmp->fxel = fexel;
  return Py_BuildValue ("O",tmp);
}


/* Append an element at the end of the fex and return it.  This
   function returns NULL if there is not enough memory to perform the
   operation.  */

static PyObject*
fex_append (fex *self, PyObject *args, PyObject *kwds)
{
  const char *fname;
  int min, max;
  fexelem *tmp = PyObject_NEW (fexelem, &fexelemType);
  rec_fex_elem_t fexel;
  static char *kwlist[] = {"fname","min","max", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zii", kwlist, &fname, &min, &max)) 
    {
      return NULL;
    }
  fexel = rec_fex_append (self->fx, fname, min, max);
  tmp->fxel = fexel;
  return Py_BuildValue ("O",tmp);
}


/* Determine whether all the elements of the given FEX are function
   calls.  */

static PyObject*
fex_all_calls_p (fex *self)
{
  bool success;
  success = rec_fex_all_calls_p (self->fx);
  return Py_BuildValue ("i",success);
}

/* Check whether a given string STR contains a proper fex description
   of type KIND.  */

static PyObject*
fex_check (fex *self, PyObject *args, PyObject *kwds)
{
  const char *str;
  enum rec_fex_kind_e kind;
  bool success;
  static char *kwlist[] = {"str","kind", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zi", kwlist, &str, &kind)) 
    {
      return NULL;
    }
  success = rec_fex_check (str, kind);
  return Py_BuildValue ("i",success);
}


/* Sort the elements of a fex using the 'min' index of the elements as
   the sorting criteria.  */
static PyObject*
fex_sort (fex *self)
{
  rec_fex_sort (self->fx);
  return Py_BuildValue ("");
}

/* Get the written form of a field expression.  This function returns
   NULL if there is not enough memory to perform the operation.  */

static PyObject*
fex_str (fex *self, PyObject *args, PyObject *kwds)
{
  enum rec_fex_kind_e kind;
  char *str;
  static char *kwlist[] = {"kind", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "i", kwlist, &kind)) 
    {
      return NULL;
    }
  str = rec_fex_str (self->fx, kind);
  return Py_BuildValue ("z",str);
}
/*record doc string */

static char fex_doc[] =
  "This type refers to the field expression structure of recutils";


static PyMethodDef fex_methods[] = {
    {"size", (PyCFunction)fex_size, METH_NOARGS,
     "Get the number of elements stored in a field expression."  
    },
    {"member_p", (PyCFunction)fex_member_p, METH_VARARGS,
     "Check whether a given field is contained in a fex."  
    },
    {"get", (PyCFunction)fex_get, METH_VARARGS,
     "Get the element of a field expression occupying the given position."  
    },
    {"append", (PyCFunction)fex_append, METH_VARARGS,
     "Append an element at the end of the fex and return it."  
    },
    {"all_calls_p", (PyCFunction)fex_all_calls_p, METH_NOARGS,
     "Determine whether all the elements of the given FEX are function calls."  
    },
    {"check", (PyCFunction)fex_check, METH_VARARGS,
     "Check whether a given string STR contains a proper fex description of type KIND."  
    },
    {"sort", (PyCFunction)fex_sort, METH_NOARGS,
     "Sort the elements of a fex."  
    },
     {"str", (PyCFunction)fex_str, METH_VARARGS,
     "Get the written form of a field expression."  
    },

    {NULL}
};


//PyDict_SetItemString(fexType, "bar", PyInt_FromLong(1));


/* Define the record object type */
static PyTypeObject fexType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.fex",              /*tp_name*/
    sizeof(fex),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)fex_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    fex_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    fex_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,      /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    fex_new,                 /* tp_new */
};


static PyObject *
field_new (PyTypeObject *type, PyObject *args, PyObject *kwds) 
{
  field *self;
  const char *name;
  const char *value;
  static char *kwlist[] = {"name","value", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zz", kwlist, &name, &value)) 
    {
      return NULL;
    }
  self = (field *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
      self->fld = rec_field_new (name,value);
      if (self->fld == NULL) 
        {
           /* Out of memory.  */
          Py_DECREF (self);
          return NULL;
        }
    }
  return (PyObject *)self;
}


static void
field_dealloc (field* self)
{
  rec_field_destroy (self->fld);
  self->ob_type->tp_free ((PyObject*)self);
}

/* Determine whether two given fields are equal (i.e. they have equal
   names but possibly different values).  */

static PyObject*
recutils_field_equal_p (PyObject *self, PyObject *args, PyObject *kwds)
{
  field *field1;
  field *field2;
  bool success;
  static char *kwlist[] = {"field1","field2", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "OO", kwlist, &field1, &field2)) 
    {
      return NULL;
    }

  success = rec_field_equal_p (field1->fld, field2->fld);
  return Py_BuildValue ("i",success);
}

/* Return a NULL terminated string containing the name of a field.
   Note that this function can't return the empty string for a
   properly initialized field.  */

static PyObject*
field_name (field *self)
{
  const char *str;
  str = rec_field_name (self->fld);
  return Py_BuildValue ("s",str);
}


/* Return a NULL terminated string containing the value of a field,
   i.e. the string stored in the field.  The returned string may be
   empty if the field has no value, but never NULL.  */

static PyObject*
field_value (field *self)
{
  const char *str;
  str = rec_field_value (self->fld);
  return Py_BuildValue ("s",str);
}


/* Set the name of a field.  This function returns 'false' if there is
   not enough memory to perform the operation.  */

static PyObject*
field_set_name (field *self, PyObject *args, PyObject *kwds)
{
  const char *name;
  static char *kwlist[] = {"name",NULL};
  bool success;
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "z", kwlist, &name)) 
    {
      return NULL;
    }
  success = rec_field_set_name (self->fld, name);
  if (!success)
    {
      PyErr_SetString (RecError, "Not enough memory to set field name");
      return NULL;
    }
  return Py_BuildValue ("i", success);
}

/* Set the value of a given field to the given string.  This function
   returns 'false' if there is not enough memory to perform the
   operation.  */

static PyObject*
field_set_value (field *self, PyObject *args, PyObject *kwds)
{
  const char *value;
  static char *kwlist[] = {"value",NULL};
  bool success;
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "z", kwlist, &value)) 
    {
      return NULL;
    }
  success = rec_field_set_value (self->fld, value);
  if (!success)
    {
      PyErr_SetString (RecError, "Not enough memory to set field value");
      return NULL;
    }
  return Py_BuildValue ("i", success);
}

/* Return a string describing the source of the field.  The specific
   meaning of the source depends on the user: it may be a file name,
   or something else.  This function returns NULL for a field for
   which a source was never set.  */

static PyObject*
field_source (field *self)
{
  const char *str;
  str = rec_field_source (self->fld);
  return Py_BuildValue ("z",str);
}


/* Set a string describing the source of the field.  Any previous
   string associated to the field is destroyed and the memory it
   occupies is freed.  This function returns 'false' if there is not
   enough memory to perform the operation.  */

static PyObject*
field_set_source (field *self, PyObject *args, PyObject *kwds)
{
  const char *source;
  static char *kwlist[] = {"source", NULL};
  bool success;
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "s", kwlist, &source)) 
    {
      return NULL;
    }
  success = rec_field_set_source (self->fld, source);
  if (!success)
    {
      PyErr_SetString (RecError, "Not enough memory to set field source");
      return NULL;
    }
  return Py_BuildValue ("i", success);
}

/* Return an integer representing the location of the field within its
   source.  The specific meaning of the location depends on the user:
   it may be a line number, or something else.  This function returns
   0 for fields not having a defined source.  */


static PyObject*
field_location (field *self)
{
  size_t loc;
  loc = rec_field_location (self->fld);
  return Py_BuildValue ("i",loc);
}

/* Return the textual representation for the location of a field
   within its source.  This function returns NULL for fields not
   having a defined source.  */

static PyObject*
field_location_str (field *self)
{
  const char *str;
  str = rec_field_location_str (self->fld);
  return Py_BuildValue ("s",str);
}


/* Return an integer representing the char location of the field
   within its source.  The specific meaning of the location depends on
   the user, usually being the offset in bytes since the beginning of
   a file or memory buffer.  This function returns 0 for fields not
   having a defined source.  */

static PyObject*
field_char_location (field *self)
{
  size_t char_loc;
  char_loc = rec_field_char_location (self->fld);
  return Py_BuildValue ("i",char_loc);
}

/* Return the textual representation for the char location of a field
   within its source.  This function returns NULL for fields not
   having a defined source.  */

static PyObject*
field_char_location_str (field *self)
{
  const char *char_loc;
  char_loc = rec_field_char_location_str (self->fld);
  return Py_BuildValue ("s",char_loc);
}


/*record doc string */
static char field_doc[] =
  "A field is an association between a label and a value.";


static PyMethodDef field_methods[] = {
    {"name", (PyCFunction)field_name, METH_NOARGS,
     "Return a NULL terminated string containing the name of a field."  
    },
    {"value", (PyCFunction)field_value, METH_NOARGS,
     "Return a NULL terminated string containing the value of a field."  
    },
    {"set_name", (PyCFunction)field_set_name, METH_VARARGS,
     "Set the name of a given field to the given string."  
    },
    {"set_value", (PyCFunction)field_set_value, METH_VARARGS,
     "Set the value of a given field to the given string."  
    },
    {"source", (PyCFunction)field_source, METH_NOARGS,
     "Return a string describing the source of the field."  
    },
    {"set_source", (PyCFunction)field_set_source, METH_VARARGS,
     "Set a string describing the source of the field."  
    },
    {"location", (PyCFunction)field_location, METH_NOARGS,
     "Return an integer representing the location of the field within its source."  
    },
    {"location_str", (PyCFunction)field_location_str, METH_NOARGS,
     "Return the textual representation of the location of the field within its source."  
    },
    {"char_location", (PyCFunction)field_char_location, METH_NOARGS,
     "Return an integer representing the char location of the field within its source. "  
    },
    {"char_location_str", (PyCFunction)field_char_location_str, METH_NOARGS,
     "Return the textual representation for the char location of a field within its source.  "  
    },
    {NULL}
};


/* Define the record object type */
static PyTypeObject fieldType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.field",              /*tp_name*/
    sizeof(field),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)field_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    field_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    field_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,      /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    field_new,                 /* tp_new */
};
  

/* Create a new comment and return it.  NULL is returned if there is
   not enough memory to perform the operation.  */  

static PyObject *
comment_new (PyTypeObject *type, PyObject *args, PyObject *kwds) 
{
  comment *self;
  char *text;
  static char *kwlist[] = {"text", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "z", kwlist, &text)) 
    {
      return NULL;
    }
  self = (comment *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
      self->cmnt = rec_comment_new (text);
      if (self->cmnt == NULL) 
        {
           /* Out of memory.  */
          Py_DECREF (self);
          return NULL;
        }
    }
  return (PyObject *)self;
}

/* Destroy a comment, freeing all used resources.  */

static void
comment_dealloc (comment* self)
{
  rec_comment_destroy (self->cmnt);
  self->ob_type->tp_free ((PyObject*)self);
}


/* Return a string containing the text in the comment.  */

static PyObject*
comment_text (comment *self)
{
  char *str;
  str = rec_comment_text (self->cmnt);
  return Py_BuildValue ("s", str);
}

/* Set the text of a comment.  Any previous text associated with the
   comment is destroyed and its memory freed.  */

static PyObject*
comment_set_text (comment *self, PyObject *args, PyObject *kwds)
{
  char *text;
  static char *kwlist[] = {"text",NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "s", kwlist, &text)) 
    {
      return NULL;
    }
  rec_comment_set_text (&self->cmnt, text);
  return Py_BuildValue ("");
} 

/* Determine whether the texts stored in two given comments are
   equal.  */

static PyObject*
recutils_comment_equal_p (PyObject *self, PyObject *args, PyObject *kwds)
{
  comment *comment1;
  comment *comment2;
  bool success;
  static char *kwlist[] = {"comment1","comment2", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "OO", kwlist, &comment1, &comment2)) 
    {
      return NULL;
    }
  success = rec_comment_equal_p (comment1->cmnt, comment2->cmnt);
  return Py_BuildValue ("i",success);
}


/*record doc string */
static char comment_doc[] =
  "A comment is a block of text.";


static PyMethodDef comment_methods[] = {
    {"text", (PyCFunction)comment_text, METH_NOARGS,
     "Return a string containing the text in the comment."  
    },
     {"set_text", (PyCFunction)comment_set_text, METH_VARARGS,
     "Set the text of a comment."  
    },
    {NULL}
};




/* Define the record object type */
static PyTypeObject commentType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.comment",              /*tp_name*/
    sizeof(comment),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)comment_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    comment_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    comment_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,      /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    comment_new,                 /* tp_new */
};



/*
 * FLEXIBLE BUFFERS
 *
 * A flexible buffer (rec_buf_t) is a buffer to which stream-like
 * operations can be applied.  Its size will grow as required.
 */

static PyObject *
buffer_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  buffer *self;
  char **data;
  size_t *size;
  static char *kwlist[] = {"data", "size", NULL};
  if (!PyArg_ParseTupleAndKeywords (args, kwds, "zi", kwlist, &data, &size)) 
    {
      return NULL;
    }
 self = (buffer *) type->tp_alloc (type, 0);
  if (self != NULL) 
    {
      self->buf = rec_buf_new (data, size);
      if (self->buf == NULL) 
        {
           /* Out of memory.  */
          Py_DECREF (self);
          return NULL;
        }
    }
  return (PyObject *)self;
}

static void
buffer_dealloc (buffer* self)
{
  //rec_com_destroy (self->cmnt);
  self->ob_type->tp_free ((PyObject*)self);
}


/*record doc string */
static char buffer_doc[] =
  "A flexible buffer is a buffer to which stream-like operations can be applied.";


static PyMethodDef buffer_methods[] = {
    {NULL}
};

/* Define the record object type */
static PyTypeObject bufferType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "recutils.buffer",              /*tp_name*/
    sizeof(buffer),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)buffer_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    buffer_doc,                 /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    buffer_methods,             /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    buffer_new,                 /* tp_new */
};


static char recutils_doc[] =
  "This module provides bindings to the librec library (GNU recutils).";


static PyMethodDef recutils_methods[] = {
    {"field_equal_p", (PyCFunction)recutils_field_equal_p, METH_VARARGS,
     "Determine whether two given fields are equal."  
    },
    {"comment_equal_p", (PyCFunction)recutils_comment_equal_p, METH_VARARGS,
     "Determine whether the texts stored in two given comments are equal."  
    },
    {NULL}  /* Sentinel */
};

/*
 * Initialization function, which is called when the module is loaded.
 */

PyMODINIT_FUNC
initrecutils (void) 
{
    PyObject* m;

    if (PyType_Ready (&recdbType) < 0)
        return;

    if (PyType_Ready (&rsetType) < 0)
        return;

    if (PyType_Ready (&recordType) < 0)
        return;

    if (PyType_Ready (&sexType) < 0)
        return;

    if (PyType_Ready (&fexType) < 0)
        return;

    if (PyType_Ready (&fieldType) < 0)
        return;  

    if (PyType_Ready (&commentType) < 0)
        return;  

    if (PyType_Ready (&bufferType) < 0)
        return; 

    m = Py_InitModule3 ("recutils", recutils_methods, recutils_doc);

    if (m == NULL)
      return;

    Py_INCREF (&recdbType);
    PyModule_AddObject (m, "recdb", (PyObject *)&recdbType);

    Py_INCREF (&rsetType);
    PyModule_AddObject (m, "rset", (PyObject *)&rsetType);

    Py_INCREF (&recordType);
    PyModule_AddObject (m, "record", (PyObject *)&recordType);

    Py_INCREF (&sexType);
    PyModule_AddObject (m, "sex", (PyObject *)&sexType);

    Py_INCREF (&fexType);
    PyModule_AddObject (m, "fex", (PyObject *)&fexType);

    Py_INCREF (&fieldType);
    PyModule_AddObject (m, "field", (PyObject *)&fieldType);

    Py_INCREF(&commentType);
    PyModule_AddObject (m, "comment", (PyObject *)&commentType);

    Py_INCREF (&bufferType);
    PyModule_AddObject (m, "buffer", (PyObject *)&bufferType);

    RecError = PyErr_NewException ("recutils.error", NULL, NULL);
    Py_INCREF (RecError);
    PyModule_AddObject (m, "error", RecError);
}
    