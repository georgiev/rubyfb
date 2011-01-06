#!/usr/bin/env ruby

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class AddRemoveUserTest < Test::Unit::TestCase
   CURDIR      = "#{Dir.getwd}"
   DB_FILE     = "#{CURDIR}#{File::SEPARATOR}add_remove_user_unit_test.fdb"

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
      sm = ServiceManager.new('localhost')
      sm.connect(DB_USER_NAME, DB_PASSWORD)

      au = AddUser.new('newuser', 'password', 'first', 'middle', 'last')
      au.execute(sm)
      sleep(3)

      cxn = @database.connect('newuser', 'password')
      cxn.close

      ru = RemoveUser.new('newuser')
      ru.execute(sm)
      sleep(3)
      
      sm.disconnect

      begin
         cxn = @database.connect('newuser', 'password')
         cxn.close
         assert(false, "Able to connect as supposedly removed user.")
      rescue FireRubyException
      end
   end
end
