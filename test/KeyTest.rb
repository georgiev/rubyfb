#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class KeyTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}key_unit_test.fdb"
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      
      # Switch to the old way of keying.
      $FireRubySettings[:ALIAS_KEYS] = false
      
      database     = Database::create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connection  = database.connect(DB_USER_NAME, DB_PASSWORD)
      @transaction = @connection.start_transaction
      @results     = ResultSet.new(@connection, @transaction,
                                   'SELECT * FROM RDB$FIELDS', 3, nil)
      @empty       = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                      0, 0, 0, 0, 0, 0, 0, 0, 0]
      
      @connection.execute_immediate('create table keytest (COL01 integer, '\
                                    'COL02 varchar(10), COL03 integer)')
      @connection.execute_immediate("insert into keytest values (1, 'Two', 3)")
   end
   
   def teardown
      @results.close
      @transaction.rollback
      @connection.close
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      $FireRubySettings[:ALIAS_KEYS] = true
      puts "#{self.class.name} finished." if TEST_LOGGING
   end
   
   def test_01
      sql  = 'select COL01 one, COL02 two, COL03 three from keytest'
      rows = @connection.execute_immediate(sql)
      data = rows.fetch
      
      count = 0
      data.each do |name, value|
         assert(['COL01', 'COL02', 'COL03'].include?(name))
         assert([1, 'Two', 3].include?(value))
         count += 1
      end
      assert(count == 3)
      
      count = 0
      data.each_key do |name|
         assert(['COL01', 'COL02', 'COL03'].include?(name))
         count += 1
      end
      assert(count == 3)
      
      count = 0
      data.each_value do |value|
         assert([1, 'Two', 3].include?(value))
         count += 1
      end
      assert(count == 3)
      
      assert(data.fetch('COL02') == 'Two')
      assert(data.fetch('COL04', 'LALALA') == 'LALALA')
      assert(data.fetch('COL00') {'NOT FOUND'} == 'NOT FOUND')
      begin
         data.fetch('COL05')
         assert(false, 'Row#fetch succeeded for non-existent column name.')
      rescue IndexError
      end
      
      assert(data.has_key?('COL01'))
      assert(data.has_key?('COL10') == false)
      
      assert(data.has_column?('COL02'))
      assert(data.has_column?('COL22') == false)
      
      assert(data.has_alias?('TWO'))
      assert(data.has_alias?('FOUR') == false)
      
      assert(data.has_value?(3))
      assert(data.has_value?('LALALA') == false)
      
      assert(data.keys.size == 3)
      data.keys.each do |name|
         assert(['COL01', 'COL02', 'COL03'].include?(name))
      end
      
      assert(data.names.size == 3)
      data.names.each do |name|
         assert(['COL01', 'COL02', 'COL03'].include?(name))
      end

      assert(data.aliases.size == 3)
      data.aliases.each do |name|
         assert(['ONE', 'TWO', 'THREE'].include?(name))
      end
      
      assert(data.values.size == 3)
      data.values.each do |value|
         assert([1, 'Two', 3].include?(value))
      end
      
      array = data.select {|name, value| name == 'COL02'}
      assert(array.size == 1)
      assert(array[0][0] == 'COL02')
      assert(array[0][1] == 'Two')
      
      array = data.to_a
      assert(array.size == 3)
      assert(array.include?(['COL01', 1]))
      assert(array.include?(['COL02', 'Two']))
      assert(array.include?(['COL03', 3]))
      
      hash = data.to_hash
      assert(hash.length == 3)
      assert(hash['COL01'] == 1)
      assert(hash['COL02'] == 'Two')
      assert(hash['COL03'] == 3)
      
      array = data.values_at('COL10', 'COL02', 'COL03')
      assert(array.size == 3)
      assert(array.include?('Two'))
      assert(array.include?(3))
      assert(array.include?(nil))
      
      rows.close
   end
end
