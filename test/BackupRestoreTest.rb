#!/usr/bin/env ruby

require './TestSetup'
require 'test/unit'
require 'rubygems'
require 'rubyfb'

include Rubyfb

class BackupRestoreTest < Test::Unit::TestCase
   DB_FILE     = File.join(DB_DIR, "backup_restore_unit_test.fdb")
   BACKUP_FILE = File.join(DB_DIR, "database.bak")

  def cleanup
    # Remove existing database files.
    if File.exist?(DB_FILE)
      db = Database.new(DB_FILE).drop(DB_USER_NAME, DB_PASSWORD)
    end

    if File.exist?(BACKUP_FILE)
      begin
        File.delete(BACKUP_FILE)
      rescue
        # ignore file permissions may cause this
      end
    end
  end

   def setup
      puts "#{self.class.name} started." if TEST_LOGGING
      cleanup

      # Create and populate the database files.
      @database = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD)
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('create table test(id integer)')
         cxn.execute_immediate('insert into test values (1000)')
         cxn.execute_immediate('insert into test values (2000)')
         cxn.execute_immediate('insert into test values (NULL)')
         cxn.execute_immediate('insert into test values (3000)')
         cxn.execute_immediate('insert into test values (4000)')
      end
   end

  def teardown
    cleanup
    puts "#{self.class.name} finished." if TEST_LOGGING
  end

   def test01
      sm = ServiceManager.new('localhost')
      sm.connect(DB_USER_NAME, DB_PASSWORD)

      b = Backup.new(DB_FILE, BACKUP_FILE)
      b.execute(sm)

      assert(File.exist?(BACKUP_FILE))
      @database.drop(DB_USER_NAME, DB_PASSWORD)
      assert(File.exists?(DB_FILE) == false)

      r = Restore.new(BACKUP_FILE, DB_FILE)
      r.execute(sm)
      sm.disconnect

      assert(File.exist?(DB_FILE))
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         total = 0
         cxn.execute_immediate('select * from test') do |row|
            assert([1000, 2000, 3000, 4000, nil].include?(row[0]))
            total += 1
         end
         assert_equal(5, total)
      end
   end

   def test02
      sm = ServiceManager.new('localhost')
      sm.connect(DB_USER_NAME, DB_PASSWORD)

      b = Backup.new(DB_FILE, BACKUP_FILE)
      b.metadata_only = true
      b.execute(sm)

      assert(File.exist?(BACKUP_FILE))
      @database.drop(DB_USER_NAME, DB_PASSWORD)
      assert(File.exists?(DB_FILE) == false)

      r = Restore.new(BACKUP_FILE, DB_FILE)
      r.execute(sm)
      sm.disconnect

      assert(File.exist?(DB_FILE))
      @database.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|
         cxn.execute_immediate('select count(*) from test') do |row|
            assert(row[0] == 0)
         end
      end
   end
end
