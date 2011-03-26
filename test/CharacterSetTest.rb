#!/usr/bin/env ruby
# encoding: windows-1251

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class CharacterSetTest < Test::Unit::TestCase
  DB_FILE  = File.join(DB_DIR, "cxnarset_unit_test.fdb")
  CHAR_SET = 'WIN1251'

  def setup
    puts "#{self.class.name} started." if TEST_LOGGING
    if File::exist?(DB_FILE)
      Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
    end
    @database = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD, 1024, CHAR_SET)
  end

  def teardown
    if File::exist?(DB_FILE)
      Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
    end
    puts "#{self.class.name} finished." if TEST_LOGGING
  end

  def test01
    db = Database.new(DB_FILE, CHAR_SET)

    assert(db.character_set = CHAR_SET)

    db.character_set = 'ASCII'
    assert(db.character_set == 'ASCII')
  end

  def test02
    db   = Database.new(DB_FILE, CHAR_SET)

    win1251_str = 'Кириличка'
    db.connect("SYSDBA", "masterkey") do |cxn|
      cxn.start_transaction do |tr|
        cxn.execute("CREATE TABLE SAMPLE_TABLE(SAMPLE_FIELD VARCHAR(100))", tr)
      end  
      cxn.start_transaction do |tr|
        cxn.execute("INSERT INTO SAMPLE_TABLE VALUES ('#{win1251_str}')", tr)
        row_count = 0
        cxn.execute("SELECT * FROM SAMPLE_TABLE WHERE SAMPLE_FIELD = '#{win1251_str}'", tr) do |row|
          assert("Database char set test failed", row['SAMPLE_FIELD']==win1251_str)
          row_count += 1
        end
        assert("Database char set test failed (WHERE clause)", 1==row_count)
      end
    end
  end
end

