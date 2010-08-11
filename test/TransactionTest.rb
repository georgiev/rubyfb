#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class TransactionTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}tx_unit_test.fdb"
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      
      @database     = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connections  = []
      @transactions = []

      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
   end
   
   def teardown
      @transactions.each do |tx|
         tx.rollback if tx.active?
      end
      @transactions.clear
      @connections.each do |cxn|
         cxn.close if cxn.open?
      end
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test01
      @transactions.push(@connections[0].start_transaction)
      assert(@transactions[0].active?)
      
      @transactions.push(Transaction.new(@connections[1]))
      assert(@transactions[1].active?)
      
      assert(@transactions[0].connections == [@connections[0]])
      assert(@transactions[1].connections == [@connections[1]])
      
      assert(@transactions[0].for_connection?(@connections[1]) == false)
      assert(@transactions[1].for_connection?(@connections[0]) == false)
      assert(@transactions[0].for_connection?(@connections[0]))
      assert(@transactions[1].for_connection?(@connections[1]))
      
      @transactions[0].commit
      assert(@transactions[0].active? == false)
      assert(@transactions[0].connections == [])
      assert(@transactions[0].for_connection?(@connections[0]) == false)
      assert(@transactions[0].for_connection?(@connections[1]) == false)
      
      @transactions[1].rollback
      assert(@transactions[1].active? == false)
      assert(@transactions[1].connections == [])
      assert(@transactions[1].for_connection?(@connections[0]) == false)
      assert(@transactions[1].for_connection?(@connections[1]) == false)
      
      @transactions[0] = Transaction.new([@connections[0], @connections[1]])
      assert(@transactions[0].active? == true)
      assert(@transactions[0].for_connection?(@connections[0]))
      assert(@transactions[0].for_connection?(@connections[1]))
      assert(@transactions[0].for_connection?(@connections[2]) == false)
      
      @transactions[0].commit
      assert(@transactions[0].active? == false)
      assert(@transactions[0].connections == [])
      assert(@transactions[0].for_connection?(@connections[0]) == false)
      assert(@transactions[0].for_connection?(@connections[1]) == false)
      assert(@transactions[0].for_connection?(@connections[2]) == false)
      
      @transactions[0] = Transaction.new([@connections[0], @connections[2]])
      assert(@transactions[0].active?)
      @transactions[0].rollback
      assert(@transactions[0].active? == false)
      assert(@transactions[0].connections == [])
      assert(@transactions[0].for_connection?(@connections[0]) == false)
      assert(@transactions[0].for_connection?(@connections[1]) == false)
      assert(@transactions[0].for_connection?(@connections[2]) == false)
   end
   
   def test02
      sql = []
      sql.push("UPDATE RDB$EXCEPTIONS SET RDB$MESSAGE = 'WoooHooo'"\
               "WHERE RDB$EXCEPTION_NAME = 'Lalala'")
      sql.push("SELECT RDB$FIELD_NAME FROM RDB$FIELDS")
      @transactions.push(Transaction.new(@connections[0]))
      
      assert(@transactions[0].execute(sql[0]) == 0)
      r = @transactions[0].execute(sql[1])
      assert(r != nil)
      assert(r.class == ResultSet)
      r.close
      
      total = 0
      @transactions[0].execute(sql[1]) do |row|
         total += 1
      end
      assert(total == 88)
   end
end
