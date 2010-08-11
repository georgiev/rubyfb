#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class ResultSetTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}result_set_unit_test.fdb"
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      @database     = Database::create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connections  = []
      @transactions = []
      
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      
      @connections[0].start_transaction do |tx|
         tx.execute("CREATE TABLE TEST_TABLE (TESTID INTEGER NOT NULL "\
                    "primary KEY, TESTINFO VARCHAR(100))")
         tx.execute('create table all_types (col01 bigint, col02 blob, '\
                    'col03 char(100), col04 date, col05 decimal(5,2), '\
                    'col06 double precision, col07 float, col08 integer, '\
                    'col09 numeric(10,3), col10 smallint, col11 time, '\
                    'col12 timestamp, col13 varchar(23))')
      end
      
      @connections[0].start_transaction do |tx|
         begin
            tx.execute("INSERT INTO TEST_TABLE VALUES (10, 'Record One.')")
            tx.execute("INSERT INTO TEST_TABLE VALUES (20, 'Record Two.')")
            tx.execute("INSERT INTO TEST_TABLE VALUES (30, 'Record Three.')")
            tx.execute("INSERT INTO TEST_TABLE VALUES (40, 'Record Four.')")
            tx.execute("INSERT INTO TEST_TABLE VALUES (50, 'Record Five.')")
         rescue Exception => error
            puts error.message
            error.backtrace.each {|step| puts "   #{step}"}
            raise
         end
      end
      
      @transactions.push(@connections[0].start_transaction)
   end
   
   def teardown
      @transactions.each do |tx|
         tx.rollback if tx.active?
      end
      @transactions.clear
      @connections.each do |cxn|
         cxn.close if cxn.open?
      end
      @connections.clear
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test01
      r = ResultSet.new(@connections[0], @transactions[0],
                        "SELECT * FROM TEST_TABLE ORDER BY TESTID", 3, nil)
      
      assert(r.connection == @connections[0])
      assert(r.transaction == @transactions[0])
      assert(r.sql == "SELECT * FROM TEST_TABLE ORDER BY TESTID")
      assert(r.dialect == 3)
      
      assert(r.fetch != nil)
      assert(r.fetch.class == Row)
      assert(r.fetch[0] == 30)
      assert(r.fetch[1] == 'Record Four.')
      r.fetch
      assert(r.fetch == nil)
      r.close
      
      r = ResultSet.new(@connections[0], @transactions[0],
                        "SELECT * FROM TEST_TABLE ORDER BY TESTID", 3, nil)
      assert(r.column_count == 2)
      assert(r.column_name(0) == 'TESTID')
      assert(r.column_name(1) == 'TESTINFO')
      assert(r.column_name(3) == nil)
      assert(r.column_name(-1) == nil)
      assert(r.column_table(0) == 'TEST_TABLE')
      assert(r.column_table(1) == 'TEST_TABLE')
      assert(r.column_table(2) == nil)
      assert(r.column_table(-1) == nil)
      assert(r.column_alias(0) == 'TESTID')
      assert(r.column_alias(1) == 'TESTINFO')
      assert(r.column_alias(3) == nil)
      assert(r.column_alias(-1) == nil)
      r.close
      
      r = ResultSet.new(@connections[0], @transactions[0],
                        "SELECT * FROM TEST_TABLE ORDER BY TESTID", 3, nil)
      total = 0
      r.each do |row|
         total += 1
      end
      assert(total == 5)
      assert(r.exhausted?)
   end

   def test02
      r = ResultSet.new(@connections[0], @transactions[0],
                        'select * from test_table where testid between ? and ?',
                        3, [20, 40])
      assert(r.exhausted? == false)
      total = 0
      r.each {|row| total += 1}
      assert(total == 3)
      assert(r.exhausted?)
   end

   def test03
      begin
         ResultSet.new(@connections[0], @transactions[0],
                       "insert into test_table values(?, ?)", 3,
                       [100, 'Should fail.'])
         assert(false, "Created result set with non-query SQL statement.")
      rescue FireRubyException
      end

      begin
         ResultSet.new(@connections[0], @transactions[0],
                       "select * from test_table where testid = ?", 3,
                       [])
         assert(false, 'Created result set with insufficient parameters.')
      rescue FireRubyException
      end
   end
   
   def test04
      results = nil
      begin
         results = @transactions[0].execute('select * from all_types')
        
         assert(results.get_base_type(0) == SQLType::BIGINT)
         assert(results.get_base_type(1) == SQLType::BLOB)
         assert(results.get_base_type(2) == SQLType::CHAR)
         assert(results.get_base_type(3) == SQLType::DATE)
         assert(results.get_base_type(4) == SQLType::DECIMAL)
         assert(results.get_base_type(5) == SQLType::DOUBLE)
         assert(results.get_base_type(6) == SQLType::FLOAT)
         assert(results.get_base_type(7) == SQLType::INTEGER)
         assert(results.get_base_type(8) == SQLType::NUMERIC)
         assert(results.get_base_type(9) == SQLType::SMALLINT)
         assert(results.get_base_type(10) == SQLType::TIME)
         assert(results.get_base_type(11) == SQLType::TIMESTAMP)
         assert(results.get_base_type(12) == SQLType::VARCHAR)
      ensure
         results.close if results != nil
      end
   end
end
