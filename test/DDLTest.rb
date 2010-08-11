#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class DDLTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}ddl_unit_test.fdb"
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      @database = Database::create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
   end
   
   def teardown
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test01
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('CREATE TABLE DDL_TABLE_01 (TABLEID '\
                               'INTEGER NOT NULL, '\
                               'FIELD01 FLOAT, FIELD02 CHAR(50), '\
                               'FIELD03 BIGINT, FIELD04 TIMESTAMP '\
                               'NOT NULL, FIELD05 VARCHAR(600))')

         cxn.start_transaction do |tx|
            r = tx.execute('SELECT COUNT(*) FROM DDL_TABLE_01')
            assert(r.fetch[0] == 0)
            r.close
         end
         
         cxn.execute_immediate('ALTER TABLE DDL_TABLE_01 ADD PRIMARY KEY '\
                               '(TABLEID)')
                            
         cxn.execute_immediate('CREATE UNIQUE INDEX DDL_TABLE_IDX ON '\
                            'DDL_TABLE_01 (TABLEID)')
                            
         cxn.execute_immediate('DROP INDEX DDL_TABLE_IDX')
      
         cxn.execute_immediate('DROP TABLE DDL_TABLE_01')
      end
   end
end
