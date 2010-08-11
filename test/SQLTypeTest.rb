#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class SQLTypeTest < Test::Unit::TestCase
   CURDIR  = "#{Dir.getwd}"
   DB_FILE = "#{CURDIR}#{File::SEPARATOR}sql_type_test.fdb"

   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end

      @database   = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @connection = @database.connect(DB_USER_NAME, DB_PASSWORD)

      @connection.start_transaction do |tx|
         tx.execute("create table all_types (col01 bigint, col02 blob, "\
                    "col03 char(100), col04 date, col05 decimal(5,2), "\
                    "col06 double precision, col07 float, col08 integer, "\
                    "col09 numeric(10,3), col10 smallint, col11 time, "\
                    "col12 timestamp, col13 varchar(23))")
      end
   end

   def teardown
      @connection.close if @connection != nil && @connection.open?

      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end
      puts "#{self.class.name} finished." if TEST_LOGGING
   end

   def test01
      types = []
      types.push(SQLType.new(SQLType::BIGINT))
      types.push(SQLType.new(SQLType::VARCHAR, 1000))
      types.push(SQLType.new(SQLType::DECIMAL, nil, 10))
      types.push(SQLType.new(SQLType::NUMERIC, nil, 5, 3))
      types.push(SQLType.new(SQLType::BLOB, nil, nil, nil, 1))

      assert(types[0] == types[0])
      assert(!(types[0] == types[1]))
      assert(types[1] == SQLType.new(SQLType::VARCHAR, 1000))

      assert(types[0].type      == SQLType::BIGINT)
      assert(types[0].length    == nil)
      assert(types[0].precision == nil)
      assert(types[0].scale     == nil)
      assert(types[0].subtype   == nil)

      assert(types[1].type      == SQLType::VARCHAR)
      assert(types[1].length    == 1000)
      assert(types[1].precision == nil)
      assert(types[1].scale     == nil)
      assert(types[1].subtype   == nil)

      assert(types[2].type      == SQLType::DECIMAL)
      assert(types[2].length    == nil)
      assert(types[2].precision == 10)
      assert(types[2].scale     == nil)
      assert(types[2].subtype   == nil)

      assert(types[3].type      == SQLType::NUMERIC)
      assert(types[3].length    == nil)
      assert(types[3].precision == 5)
      assert(types[3].scale     == 3)
      assert(types[3].subtype   == nil)

      assert(types[4].type      == SQLType::BLOB)
      assert(types[4].length    == nil)
      assert(types[4].precision == nil)
      assert(types[4].scale     == nil)
      assert(types[4].subtype   == 1)
   end

   def test02
      types = SQLType.for_table("all_types", @connection)

      assert(types['COL01'] == SQLType.new(SQLType::BIGINT))
      assert(types['COL02'] == SQLType.new(SQLType::BLOB, nil, nil, nil, 0))
      assert(types['COL03'] == SQLType.new(SQLType::CHAR, 100))
      assert(types['COL04'] == SQLType.new(SQLType::DATE))
      assert(types['COL05'] == SQLType.new(SQLType::DECIMAL, nil, 5, 2))
      assert(types['COL06'] == SQLType.new(SQLType::DOUBLE))
      assert(types['COL07'] == SQLType.new(SQLType::FLOAT))
      assert(types['COL08'] == SQLType.new(SQLType::INTEGER))
      assert(types['COL09'] == SQLType.new(SQLType::NUMERIC, nil, 10, 3))
      assert(types['COL10'] == SQLType.new(SQLType::SMALLINT))
      assert(types['COL11'] == SQLType.new(SQLType::TIME))
      assert(types['COL12'] == SQLType.new(SQLType::TIMESTAMP))
      assert(types['COL13'] == SQLType.new(SQLType::VARCHAR, 23))
   end
end
