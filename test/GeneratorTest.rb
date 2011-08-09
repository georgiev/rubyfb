#!/usr/bin/env ruby

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class GeneratorTest < Test::Unit::TestCase
   DB_FILE = File.join(DB_DIR, "generator_unit_test.fdb")
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      @database     = Database::create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connections  = []
      
      @connections.push(@database.connect(DB_USER_NAME, DB_PASSWORD))
   end
   
   def teardown
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
      assert(!Generator::exists?('TEST_GEN', @connections[0]))
      g = Generator::create('TEST_GEN', @connections[0])
      assert(Generator::exists?('TEST_GEN', @connections[0]))
      assert_equal(0, g.last)
      assert_equal(1, g.next(1))
      assert_equal(1, g.last)
      assert_equal(11, g.next(10))
      assert_equal(@connections[0], g.connection)
      assert_equal('TEST_GEN', g.name)
      
      g.drop
      assert(!Generator::exists?('TEST_GEN', @connections[0]))
   end
end
