#!/usr/bin/env ruby
# encoding: utf-8

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class FieldCharacterSetTest < Test::Unit::TestCase
  DB_FILE  = File.join(DB_DIR, "cxnarset_unit_test.fdb")
  DB_CHAR_SET = 'NONE'

  def setup
    puts "#{self.class.name} started." if TEST_LOGGING
    if File::exist?(DB_FILE)
      Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
    end
    @database = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD, 1024, DB_CHAR_SET)
  end

  def teardown
    if File::exist?(DB_FILE)
      Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
    end
    puts "#{self.class.name} finished." if TEST_LOGGING
  end

  def test01
    db   = Database.new(DB_FILE, DB_CHAR_SET)
    utf8_str = "Малко utf8 кирилица"
    db.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
      cxn.start_transaction do |tr|
        cxn.execute("CREATE TABLE SAMPLE_TABLE(SAMPLE_FIELD VARCHAR(100) CHARACTER SET UTF8)", tr)
      end  
      cxn.start_transaction do |tr|
        cxn.execute("INSERT INTO SAMPLE_TABLE VALUES ('#{utf8_str}')", tr)
        row_count = 0
        cxn.execute("SELECT * FROM SAMPLE_TABLE WHERE SAMPLE_FIELD = '#{utf8_str}'", tr) do |row|
          assert(row['SAMPLE_FIELD'] == utf8_str, "Field encoding failed")
          row_count += 1
        end
        assert(1==row_count)
      end
    end
  end
end
