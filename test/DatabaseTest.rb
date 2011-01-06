#!/usr/bin/env ruby

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class DatabaseTest < Test::Unit::TestCase
   CURDIR      = "#{Dir.getwd}"
   DB_FILE     = "#{CURDIR}#{File::SEPARATOR}db_unit_test.fdb"
   CREATE_FILE = "#{CURDIR}#{File::SEPARATOR}db_create_test.fdb"
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File.exist?(DB_FILE)
         db = Database.new(DB_FILE)
         db.drop(DB_USER_NAME, DB_PASSWORD)
      end
      if File.exist?(CREATE_FILE)
         db = Database.new(CREATE_FILE)
         db.drop(DB_USER_NAME, DB_PASSWORD)
      end
      
      Database::create(DB_FILE, DB_USER_NAME, DB_PASSWORD, 1024, 'ASCII')
   end
   
   def teardown
      if File::exist?(DB_FILE)
         db = Database.new(DB_FILE)
         db.drop(DB_USER_NAME, DB_PASSWORD)
      end
      if File::exist?(CREATE_FILE)
         db = Database.new(CREATE_FILE)
         db.drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   
   def test01
      db = Database.create(CREATE_FILE, DB_USER_NAME, DB_PASSWORD, 2048, 'ASCII')
      
      assert(File.exist?(CREATE_FILE))
      assert(db.file == CREATE_FILE)
      
      begin
         Database.create(CREATE_FILE, DB_USER_NAME, DB_PASSWORD, 2048, 'ASCII')
         assert(false,
                "Successfully created a database file that already exists.")
      rescue Exception
      end
      
      db.drop(DB_USER_NAME, DB_PASSWORD)
      assert(File.exist?(CREATE_FILE) == false)
   end
   
   def test02
      db = Database.new(DB_FILE);
      
      assert(db.file == DB_FILE)
      
      c = db.connect(DB_USER_NAME, DB_PASSWORD)
      assert(c != nil)
      c.close
   end
   
   def test03
      db = Database.new(DB_FILE);
      c  = nil

      db.connect(DB_USER_NAME, DB_PASSWORD) do |connection|
         assert(connection != nil)
         assert(connection.class == Connection)
         assert(connection.open?)
         c = connection
      end
      assert(c != nil)
      assert(c.class == Connection)
      assert(c.closed?)
   end
end
