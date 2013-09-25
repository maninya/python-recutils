#!/usr/bin/python
import sys	
import recutils
import pyrec

    
db = pyrec.Recdb()
db1 = pyrec.Recdb()
db.loadfile("movies.rec")

print "\nCREATE THE SEXES & FEXES"
sex1 = pyrec.RecSex(1)
b = sex1.pycompile("Audio = 'German'")
print "Sex compiled success = ",b

print "\nCALLING QUERY FUNCTION"
print "Query for a record set consisting of a list of German language movies"
queryrset = db.query("movies", None, None, sex1, None, 10, None, None, None, None, 0)
num_rec = queryrset.num_records()
print "Number of queried records = ",num_rec

print "\nINSERTING QUERIED RSET"
db1.insert_rset(queryrset,2);

db1.writefile("german.rec")