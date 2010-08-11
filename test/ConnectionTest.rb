#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class ConnectionTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}con_unit_test.fdb"
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end

      @database    = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connections = []
   end
   
   def teardown
      @connections.each {|connection| connection.close}
      @connections.clear

      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test01
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      
      assert(@connections[0].user == DB_USER_NAME)
      assert(@connections[0].open?)
      assert(@connections[0].closed? == false)
      assert(@connections[0].database == @database)
      assert(@connections[0].to_s == "#{DB_USER_NAME}@#{DB_FILE} (OPEN)")
      
      @connections[0].close
      assert(@connections[0].user == DB_USER_NAME)
      assert(@connections[0].open? == false)
      assert(@connections[0].closed?)
      assert(@connections[0].database == @database)
      assert(@connections[0].to_s == "(CLOSED)")
      
      @connections[0] = @database.connect(DB_USER_NAME, DB_PASSWORD)
      assert(@connections[0].open?)
      @connections.push(Connection.new(@database, DB_USER_NAME, DB_PASSWORD))
      assert(@connections[0].open?)
      assert(@connections[1].open?)
   end
   
   def test02
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      
      tx = @connections[0].start_transaction
      assert(tx != nil)
      assert(tx.class == Transaction)
      assert(tx.active?)
      tx.rollback
      
      tx = nil
      @connections[0].start_transaction do |transaction|
         assert(transaction != nil)
         assert(transaction.class == Transaction)
         assert(transaction.active?)
         tx = transaction
      end
      assert(tx != nil)
      assert(tx.class == Transaction)
      assert(tx.active? == false)
   end
   
   def test03
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
      tx    = @connections[0].start_transaction
      total = 0
      @connections[0].execute("SELECT RDB$FIELD_NAME FROM RDB$FIELDS", tx) do |row|
         total = total + 1
      end
      assert(total == 88)
      tx.commit

      total = 0      
      @connections[0].execute_immediate("SELECT RDB$FIELD_NAME FROM RDB$FIELDS") do |row|
         total = total + 1
      end
      assert(total == 88)
   end

   def test04
      connection = @database.connect(DB_USER_NAME, DB_PASSWORD)

      tx1 = connection.start_transaction
      tx2 = connection.start_transaction
      tx3 = connection.start_transaction

      tx2.rollback
      assert(tx2.active? == false)

      connection.close

      assert(connection.closed?)
      assert(tx1.active? == false)
      assert(tx3.active? == false)
   end
end
