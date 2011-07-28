#!/usr/bin/env ruby
# encoding: utf-8

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class BlobTest < Test::Unit::TestCase
   DB_FILE     = File.join(DB_DIR, "blob_unit_test.fdb")
   TXT_FILE    = File.join(DB_DIR, "data.txt")
   DATA        = "aasdfjakhdsfljkashdfslfhaslhasyhfawyufalwuhlhsdlkfhasljlkshflalksjhasjhalsjhdf\nasdflkajshdfjkahsdfjajdfalsdfasdf\nasdfasdfasdkljfhajdfhkjasdfagdsflalkjfgagsdflasdf\nasdfasdfasdf"
   UTF_DATA        = "ред с utf кирилица"

   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      # Remove existing database files.
      @database = Database.new(DB_FILE)
      if File.exist?(DB_FILE)
         @database.drop(DB_USER_NAME, DB_PASSWORD)
      end
      Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
   end

   def teardown
      # Remove existing database files.
      if File.exist?(DB_FILE)
         @database.drop(DB_USER_NAME, DB_PASSWORD)
      end

      puts "#{self.class.name} finished." if TEST_LOGGING
   end

   def test01
      d = nil
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('create table blob_test (data blob sub_type 0)')
         cxn.start_transaction do |tx|

            s = Statement.new(cxn, tx, 'INSERT INTO BLOB_TEST VALUES(?)', 3)
            s.execute_for([DATA], tx)
            
            # Perform a select of the value inserted.
            r = cxn.execute('SELECT * FROM BLOB_TEST', tx)
            d = r.fetch
            
            assert_equal(d[0].to_s, DATA)

            # Clean up.
            s.close
            r.close
         end
         cxn.execute_immediate('DROP TABLE BLOB_TEST')
      end
   end

   def test02
      d = nil
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('create table blob_test (data blob sub_type 1 segment size 10 CHARACTER SET UTF8)')
         cxn.start_transaction do |tx|

            s = Statement.new(cxn, tx, 'INSERT INTO BLOB_TEST VALUES(?)', 3)
            s.execute_for([UTF_DATA], tx)
            
            # Perform a select of the value inserted.
            r = cxn.execute('SELECT * FROM BLOB_TEST', tx)
            d = r.fetch
            
            assert_equal(UTF_DATA, d[0].to_s)

            # Clean up.
            s.close
            r.close
         end
         cxn.execute_immediate('DROP TABLE BLOB_TEST')
      end
   end
   
end
