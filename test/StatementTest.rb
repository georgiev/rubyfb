#!/usr/bin/env ruby
# encoding: utf-8

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class StatementTest < Test::Unit::TestCase
   DB_FILE = File.join(DB_DIR, "stmt_unit_test.fdb")
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      @database     = Database::create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connections  = []
      @transactions = []
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
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      @transactions.push(@connections.last.start_transaction)
      
      s1 = @connections[0].create_statement("SELECT RDB$FIELD_NAME FROM RDB$FIELDS")
      assert(s1 != nil)
      assert(s1.sql == "SELECT RDB$FIELD_NAME FROM RDB$FIELDS")
      assert(s1.connection == @connections[0])
      assert(s1.dialect == 3)
      assert(s1.type == Statement::SELECT_STATEMENT)
      s1.close
      
      s2 = @connections[0].create_statement("DELETE FROM RDB$EXCEPTIONS")
      assert(s2 != nil)
      assert(s2.sql == "DELETE FROM RDB$EXCEPTIONS")
      assert(s2.connection == @connections[0])
      assert(s2.dialect == 3)
      assert(s2.type == Statement::DELETE_STATEMENT)
      s2.close
   end
   
   def test02
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      @transactions.push(@connections[0].start_transaction)
      
      s = @connections[0].create_statement(
                        "SELECT RDB$FIELD_NAME FROM RDB$FIELDS "\
                        "WHERE RDB$FIELD_NAME LIKE ?")

      begin
         r = s.execute(@transactions[0])
         r.close
         assert(false,
                "Executed statement that required parameter without the "\
                "parameter being specified.")
      rescue Exception => error
      end
      
      begin
         r = s.exec([], @transactions[0])
         r.close
         assert(false,
                "Executed statement that required a parameter with an empty "\
                "parameter set.")
      rescue Exception => error
      end
      
      begin
         r = s.exec(['LALALA', 25], @transactions[0])
         r.close
         assert(false,
                "Executed statement that required a parameter with too many "\
                "parameters.")
      rescue Exception => error
      end
      
      r = s.exec(['LALALA'], @transactions[0])
      assert_not_nil(r)
      assert_equal(ResultSet, r.class)
      r.close
      
      total = 0
      s.exec(["RDB$COLLATION%"], @transactions[0]) do |row|
         total += 1
      end
      assert_equal(2, total)
      s.close
   end
   
   def test03
      d = nil
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('CREATE TABLE STRING_TEST(TEXT VARCHAR(10))')
         cxn.start_transaction do |tx|
            # Perform an truncated insert.
            s = cxn.create_statement('INSERT INTO STRING_TEST VALUES(?)')
            s.exec(['012345678901234'], tx)
            
            # Perform a select of the value inserted.
            r = cxn.execute('SELECT * FROM STRING_TEST', tx)
            d = r.fetch
            
            # Clean up.
            s.close
            r.close
         end
         assert(d[0] == '0123456789')
         cxn.execute_immediate('DROP TABLE STRING_TEST')
      end
   end
   
   def test04
      d = nil
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         utf_str = 'utf кирилица';
         cxn.execute_immediate('CREATE TABLE STRING_TEST(TEXT VARCHAR(100) CHARACTER SET UTF8)')
         cxn.start_transaction do |tx|
            # Perform an truncated insert.
            s = cxn.create_statement('INSERT INTO STRING_TEST VALUES(?)')
            s.exec([utf_str], tx)
            
            # Perform a select of the value inserted.
            r = cxn.execute('SELECT * FROM STRING_TEST', tx)
            d = r.fetch
            
            # Clean up.
            s.close
            r.close
         end
         assert(d[0] == utf_str)
         cxn.execute_immediate('DROP TABLE STRING_TEST')
      end
   end
   
  def test05
    @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
      cxn.execute_immediate('CREATE TABLE PLAN_TEST(TEXT VARCHAR(100) CHARACTER SET UTF8)')
      s = cxn.create_statement('SELECT * FROM PLAN_TEST')
      assert_equal("PLAN (PLAN_TEST NATURAL)", s.plan)
      s.close
      cxn.execute_immediate('DROP TABLE PLAN_TEST')
    end
  end
end
