#!/usr/bin/env ruby

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class RoleTest < Test::Unit::TestCase
   DB_FILE     = File.join(DB_DIR, "role_unit_test.fdb")

   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      # Remove existing database files.
      @database = Database.new(DB_FILE)
      if File.exist?(DB_FILE)
         @database.drop(DB_USER_NAME, DB_PASSWORD)
      end
      Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)

      @sm = ServiceManager.new('localhost')
      @sm.connect(DB_USER_NAME, DB_PASSWORD)

      au = AddUser.new('user1', 'password', 'first', 'middle', 'last')
      au.execute(@sm)
      sleep(3)
   end

   def teardown
      ru = RemoveUser.new('user1')
      ru.execute(@sm)
      sleep(3)

      @sm.disconnect

      # Remove existing database files.
      if File.exist?(DB_FILE)
         @database.drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end

   def test01
      cxn = @database.connect(DB_USER_NAME, DB_PASSWORD)
      cxn.execute_immediate('CREATE TABLE TEST (field1 INTEGER)')
      cxn.execute_immediate('CREATE role myrole')
      cxn.execute_immediate('GRANT myrole to user1')
      cxn.execute_immediate('GRANT ALL on TEST to myrole')
      cxn.close

      cxn = @database.connect('user1', 'password')      
      assert_raise FireRubyException do
        t1 = cxn.start_transaction
        r = cxn.execute("select * from test", t1)
        r.fetch
        r.close
        t1.commit
      end
      cxn.close

      cxn = @database.connect('user1', 'password', {Connection::SQL_ROLE_NAME => "myrole"})
      assert_nothing_raised do
        t1 = cxn.start_transaction
        r = cxn.execute("select * from test", t1)
        r.fetch
        r.close
        t1.commit
      end
      cxn.close
   end
end
