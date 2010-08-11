#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class ServiceManagerTest < Test::Unit::TestCase
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
   end
   
   def teardown
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test01
      sm = ServiceManager.new('localhost')
      assert(sm.connected? == false)

      assert(sm.connect(DB_USER_NAME, DB_PASSWORD) == sm)
      assert(sm.connected?)

      assert(sm.disconnect == sm)
      assert(sm.connected? == false)
   end
end
