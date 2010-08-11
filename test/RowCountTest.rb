#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class RowCountTest < Test::Unit::TestCase
   CURDIR      = "#{Dir.getwd}"
   DB_FILE     = "#{CURDIR}#{File::SEPARATOR}row_count_test.fdb"

   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      # Create the database for use in testing.
      if File.exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      @database = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      
      # Create the test table.
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('create table test(id integer)')
      end
   end
   
   
   def teardown
      if File.exist?(DB_FILE)
         @database.drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test01
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.start_transaction do |tx|
            assert(cxn.execute_immediate('insert into test values (1000)') == 1)
            assert(cxn.execute_immediate('insert into test values (1000)') == 1)
            assert(cxn.execute('insert into test values (2000)', tx) == 1)
            assert(cxn.execute('insert into test values (2000)', tx) == 1)
            assert(tx.execute('insert into test values (3000)') == 1)
            assert(tx.execute('insert into test values (3000)') == 1)
            assert(tx.execute('insert into test values (4000)') == 1)
            assert(tx.execute('insert into test values (4000)') == 1)
            
            assert(cxn.execute_immediate('update test set id = 10000 where '\
                                         'id = 1000') == 2)
            assert(cxn.execute('update test set id = 20000 where id = 2000',
                               tx) == 2)
            assert(tx.execute('update test set id = 30000 where id = 3000') == 2)
            
            s = Statement.new(cxn, tx, 'update test set id = 40000 where id = ?',
                              3)
            assert(s.execute_for([4000]) == 2)

            
            assert(cxn.execute_immediate('delete from test where id = 10000') == 2)
            assert(cxn.execute('delete from test where id = 20000', tx) == 2)
            assert(tx.execute('delete from test where id = 30000') == 2)
         end
      end
   end
end
