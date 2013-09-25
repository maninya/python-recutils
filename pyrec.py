#!/usr/bin/python
import sys
import recutils
import os, errno

class Recdb(recutils.recdb):
 	def __init__(self):
		pass

	def loadfile(self, filename):
		try:
			self.pyloadfile(filename)	
		except recutils.error as e:
			print 'File load failed:', e

	def writefile(self, filename):
		try:
			self.pywritefile(filename)	
		except recutils.error as e:
			print 'File write failed:', e

	def appendfile(self, filename):
		try:
			self.pyappendfile(filename)	
		except recutils.error as e:
			print 'File append failed:', e

	def insert_rset(self, recset, position):
		try:
			self.pyinsert_rset(recset, position)	
		except recutils.error as e:
			print e

	def remove_rset(self, position):
		try:
			self.pyremove_rset(position)	
		except recutils.error as e:
			print e
	
	
class Fexenum(recutils.fex):
 	(REC_FEX_SIMPLE, REC_FEX_CSV, REC_FEX_SUBSCRIPTS) = range(0,3)

class RecSetenum(recutils.recdb):
 	(REC_SET_ACT_NONE, REC_SET_ACT_RENAME, 
 	 REC_SET_ACT_SET, REC_SET_ACT_ADD, 
 	 REC_SET_ACT_SETADD, REC_SET_ACT_DELETE, REC_SET_ACT_COMMENT) = range(0,7)



class RecSex(recutils.sex):
	 	
	def compile(self, expr):
		try:
			self.pycompile(expr)	
		except recutils.error as e:
			print e

	def eval(self, rec, status):
		try:
			self.pyeval(rec, status)	
		except recutils.error as e:
			print e


		
	

