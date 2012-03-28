#!/usr/bin/env ruby

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'
require 'date'

include Rubyfb

class TypeTest < Test::Unit::TestCase
   DB_FILE     = File.join(DB_DIR, "types_unit_test.fdb")
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File.exist?(DB_FILE)
         db = Database.new(DB_FILE)
         db.drop(DB_USER_NAME, DB_PASSWORD)
      end
      @db  = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      cxn = @db.connect(DB_USER_NAME, DB_PASSWORD)
      cxn.start_transaction do |tx|
         tx.execute("create table types_table (COL01 integer, "\
                    "COL02 float, COL03 decimal(10,2), "\
                    "COL04 numeric(5,3), COL05 date, COL06 timestamp, "\
                    "COL07 char(10), COL08 time, COL09 varchar(30), COL10 decimal(9,2))")
      end
         
      cxn.start_transaction do |tx|
         stmt = cxn.create_statement("insert into types_table values "\
                                       "(?, ?, ?, ?, ?, ?, ?, ?, ?, ?)")
         stmt.exec([10, 100.2, 2378.65, 192.345,
                           Date.new(2005, 10, 21), Time.new, 'La la la',
                           Time.new, "Oobly Joobly", "2530.70".to_f], tx)
         stmt.close
      end
      cxn.close
   end
   
   def teardown
      if File::exist?(DB_FILE)
         @db.drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end

   def test01
      rows = cxn = nil
      begin
         cxn = @db.connect(DB_USER_NAME, DB_PASSWORD)
         rows = cxn.execute_immediate('select * from types_table')
         row  = rows.fetch
         assert(row[0].kind_of?(Integer))
         assert(row[1].instance_of?(Float))
         assert(row[2].instance_of?(Float))
         assert(row[3].instance_of?(Float))
         assert(row[4].instance_of?(Date))
         assert(row[5].instance_of?(Time))
         assert(row[6].instance_of?(String))
         assert(row[7].instance_of?(Time))
         assert(row[8].instance_of?(String))
         assert_equal("2530.70".to_f, row[9])
      ensure
         rows.close if rows != nil
         cxn.close if cxn != nil
      end
   end

   def test02
      $FireRubySettings[:DATE_AS_DATE] = false
      rows = cxn = nil
      begin
         cxn = @db.connect(DB_USER_NAME, DB_PASSWORD)
         rows = cxn.execute_immediate('select * from types_table')
         row  = rows.fetch
         assert(row[0].kind_of?(Integer))
         assert(row[1].instance_of?(Float))
         assert(row[2].instance_of?(Float))
         assert(row[3].instance_of?(Float))
         assert(row[4].instance_of?(Time), "Time expected but #{row[4].class.name} found")
         assert(row[5].instance_of?(Time))
         assert(row[6].instance_of?(String))
         assert(row[7].instance_of?(Time))
         assert(row[8].instance_of?(String))
         assert_equal("2530.70".to_f, row[9])
      ensure
         rows.close if rows != nil
         cxn.close if cxn != nil
      end
      $FireRubySettings[:DATE_AS_DATE] = true
   end
end
