#!/usr/bin/env ruby
# encoding: utf-8

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class StoredProcedureTest < Test::Unit::TestCase
   DB_FILE = File.join(DB_DIR, "sp_unit_test.fdb")
   
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
      d = nil
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('
            create procedure SP1 (
              SP_IN_1 VARCHAR(10),
              SP_IN_2 integer
            )
            returns (
              SP_OUT_1 VARCHAR(20),
              SP_OUT_2 INTEGER
            )
            as
            begin
              SP_OUT_1 = :SP_IN_1;
              SP_OUT_2 = SP_IN_2;
            end
         ')
         cxn.start_transaction do |tx|
            # Perform an truncated insert.
            test_data = ["SP1", 7]
            
            s = cxn.create_statement('execute procedure SP1(?, ?)')
            result = s.execute_for(test_data, tx)
            
            assert_equal(Rubyfb::ResultSet, result.class)
            row = result.fetch
            assert_not_nil(row)
            assert_nil(result.fetch)
            assert_equal(result.exhausted?, true)

            assert_equal(test_data[0], row[0])
            assert_equal(test_data[1], row[1])
            
            # Clean up.
            result.close
            s.close
         end
         cxn.execute_immediate('DROP PROCEDURE SP1')
      end
   end
end
