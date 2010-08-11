#!/usr/bin/env ruby

require 'TestSetup'
require 'test/unit'
require 'rubygems'
require 'fireruby'

include FireRuby

class CharacterSetTest < Test::Unit::TestCase
   CURDIR   = "#{Dir.getwd}"
   DB_FILE  = "#{CURDIR}#{File::SEPARATOR}cxnarset_unit_test.fdb"
   CHAR_SET = 'WIN1251'
   
   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      if File::exist?(DB_FILE)
         Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
      end

      @database = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD, 1024,
                                  CHAR_SET)
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
      text = "•?‰Œ˜"
      db   = Database.new(DB_FILE, CHAR_SET)

      begin
         db.connect("SYSDBA", "masterkey") do |cxn|
            cxn.start_transaction do |tr|
               cxn.execute("CREATE TABLE SAMPLE_TABLE(SAMPLE_FIELD VARCHAR(100))",tr)
            end  
            cxn.start_transaction do |tr|
               cxn.execute("INSERT INTO SAMPLE_TABLE VALUES ('#{win1251_str}')",tr)
               cxn.execute("SELECT * FROM SAMPLE_TABLE WHERE SAMPLE_FIELD = "\
                           "'#{win1251_str}'",tr) do |row|
                  # here we have an exception:
                  some_var = row['SAMPLE_FIELD']
               end
            end
         end
      rescue => error
         assert("Character set unit test failure.", false)
      end
   end
end
