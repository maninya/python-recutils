#!/usr/bin/python
import sys	
import recutils
import pyrec

print "CREATING DATABASES"
db = pyrec.Recdb()
db.appendfile("books.rec")
db.appendfile("account.rec")
db.appendfile("account.rec") #Should get duplicate rset error
db.writefile("books_account.rec")
print "Created db"
db2 = pyrec.Recdb()
db2.loadfile("books.rec")
print "Created db2"

print "CREATE TWO FIELDS"
fl1 = recutils.field("Author", "Richard M. Stallman")
fl2 = recutils.field("Skater", "Terry Pratchett")

ch = recutils.field_equal_p(fl1,fl2)
print ch
if ch:
	print "Fields equal"
else:
	print "Fields not equal"

print "\nPRINT NAME AND VALUE OF FIELDS"
name1 = fl1.name()
print "name1 = ",name1

fvalue2 = fl2.value()
print "fvalue2 = ",fvalue2

print "\nSET NAME OF FIELD1"
s = fl1.set_name("Mike Wazowski")

name1 = fl1.name()
print "New name1 = ",name1

print "\nSET VALUE OF FIELD2"
s = fl2.set_value("The Friendly Monster")

value2 = fl2.value()
print "New value2 = ",value2

print "\nGET SOURCE OF FIELD2"
source = fl2.source()
print "source = ", source

print "\nGETTING THE RECORD SET AT A CERTAIN POSITION"
recset = db.get_rset(2)
print "Got the rset"

print "\nGETTING THE RECORD DESCRIPTOR OF AN RSET"
desc = recset.descriptor()
print "Got the record descriptor"

print "\nCHECKING IF FIELD '%confidential: Password' EXISTS"
fname = desc.contains_field("%confidential", "Password")
if fname:
	print "Field exists"
else:
	print "Field doesn't exist"

print "\nINSERT AN RSET INTO DB"
num = db2.size()
print "Size before = ",num
db2.insert_rset(recset,0);
num = db2.size()
print "Size after = ",num
print "Writing to file"
flag4 = db2.pywritefile("account_books.rec")
print "flag4 = ", flag4

print "\nREMOVE AN RSET FROM DB"
db2.remove_rset(2)
print "Writing to file"
flag4 = db2.pywritefile("account1.rec")
print "flag4 = ", flag4


print "\nCREATE THE SEXES & FEXES"
sex1 = pyrec.RecSex(1)
b = sex1.pycompile("Location = 'home'")
print "Sex compiled success = ",b
fexe1 = pyrec.Fexenum.REC_FEX_SIMPLE
fex1 = recutils.fex("Author",fexe1)

print "\nCALLING QUERY FUNCTION"
print "Query for a record set consisting of the list of Authors of books at home"
queryrset = db.query("Book", None, None, sex1, None, 10, fex1, None, None, None, 0)
num_rec = queryrset.num_records()
print "Number of queried records = ",num_rec

print "\nINSERTING QUERIED RSET"
db2.insert_rset(queryrset,2);

print "\nSIZE AFTER INSERTING"
num = db2.size()
print "Num = ",num

flag4 = db2.writefile("account1_query.rec")



print "\nCALLING INSERT FUNCTION"
print "Inserting the record descriptor of Account file"
ins = db.insert("Account", None, None, None, 1, None, desc, 0)
print "Insert success? - check \"books_account_ins.rec\"", ins
flag4 = db.writefile("books_account_ins.rec")


print "\nCALLING DELETE FUNCTION"
print "Deleting the record descriptor of Account file"
ins = db.delete("Account", None, None, None, 1, 0)
print "Delete success? - check \"books_account_del.rec\"", ins
flag4 = db.writefile("books_account_del.rec")

print "\nCALLING SET FUNCTION"
print "Change the authors of all books at home to J.R.R. Tolkien"

db3 = pyrec.Recdb()
db3.loadfile("books.rec")
print "Created db3"
rsetenum = pyrec.RecSetenum.REC_SET_ACT_SETADD
print "rsetenum = ",rsetenum
ins = db3.set("Book", None, sex1, None, 10, fex1, rsetenum, "J.R.R.Tolkien", 0)
print "Set success? - check \"books_tolkien.rec\"", ins
flag4 = db3.writefile("books_tolkien.rec")


print "\nINTEGRITY CHECK OF MOVIES.REC FILE - added %mandatory: Date"
db4 = pyrec.Recdb()
db4.loadfile("movies.rec")
print "Created db4"
err = recutils.buffer("hello",100)
n = db4.int_check(1,1,err)
print "Number of errors = ",n

