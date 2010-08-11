#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class StatementTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}stmt_unit_test.fdb"
   
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
      
      s1 = Statement.new(@connections[0],
                         @transactions[0],
                         "SELECT RDB$FIELD_NAME FROM RDB$FIELDS", 3)
      assert(s1 != nil)
      assert(s1.sql == "SELECT RDB$FIELD_NAME FROM RDB$FIELDS")
      assert(s1.connection == @connections[0])
      assert(s1.transaction == @transactions[0])
      assert(s1.dialect == 3)
      assert(s1.type == Statement::SELECT_STATEMENT)
      s1.close
      
      s2 = Statement.new(@connections[0],
                         @transactions[0],
                         "DELETE FROM RDB$EXCEPTIONS", 1)
      assert(s2 != nil)
      assert(s2.sql == "DELETE FROM RDB$EXCEPTIONS")
      assert(s2.connection == @connections[0])
      assert(s2.transaction == @transactions[0])
      assert(s2.dialect == 1)
      assert(s2.type == Statement::DELETE_STATEMENT)
      s2.close
   end
   
   def test02
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      @transactions.push(@connections[0].start_transaction)
      
      s = Statement.new(@connections[0], @transactions[0],
                        "SELECT RDB$FIELD_NAME FROM RDB$FIELDS "\
                        "WHERE RDB$FIELD_NAME LIKE ?", 3)

      begin
         r = s.execute
         r.close
         assert(false,
                "Executed statement that required parameter without the "\
                "parameter being specified.")
      rescue Exception => error
      end
      
      begin
         r = s.execute_for([])
         r.close
         assert(false,
                "Executed statement that required a parameter with an empty "\
                "parameter set.")
      rescue Exception => error
      end
      
      begin
         r = s.execute_for(['LALALA', 25])
         r.close
         assert(false,
                "Executed statement that required a parameter with too many "\
                "parameters.")
      rescue Exception => error
      end
      
      r = s.execute_for(['LALALA'])
      assert(r != nil)
      assert(r.class == ResultSet)
      r.close
      
      total = 0
      s.execute_for(["%"]) do |row|
         total = total + 1
      end
      assert(total = 88)
      s.close
   end
   
   def test03
      d = nil
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('CREATE TABLE STRING_TEST(TEXT VARCHAR(10))')
         cxn.start_transaction do |tx|
            # Perform an truncated insert.
            s = Statement.new(cxn, tx, 'INSERT INTO STRING_TEST VALUES(?)', 3)
            s.execute_for(['012345678901234'])
            
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
end
