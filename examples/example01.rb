#!/usr/bin/env ruby

require 'rubygems'
require_gem 'fireruby'

include FireRuby

# Database details constants.
DB_FILE      = "localhost:#{File.expand_path('.')}#{File::SEPARATOR}example.fdb"
DB_USER_NAME = "sysdba"
DB_PASSWORD  = "masterkey"

# SQL constants.
CREATE_TABLE_SQL = 'CREATE TABLE TESTTABLE (TESTID INTEGER NOT NULL PRIMARY '\
                   'KEY, TESTTEXT VARCHAR(100), TESTFLOAT NUMERIC(6,2), '\
                   'CREATED TIMESTAMP)'
DROP_TABLE_SQL   = 'DROP TABLE TESTTABLE'
INSERT_SQL       = 'INSERT INTO TESTTABLE VALUES(?, ?, ?, ?)'
SELECT_SQL       = 'SELECT * FROM TESTTABLE'

begin
   # Check if the database file exists.
   db = nil
   if File.exist?(DB_FILE) == false
      # Create the database file.
      db = Database.create(DB_FILE, DB_USER_NAME, DB_PASSWORD, 1024, 'ASCII')
   else
      # Create the databse object.
      db = Database.new(DB_FILE)
   end
   
   # Obtain a connection to the database.
   db.connect(DB_USER_NAME, DB_PASSWORD) do |cxn|   
      # Create the database table.
      cxn.execute_immediate(CREATE_TABLE_SQL)
      
      # Insert 50 rows into the database.
      decimal = 1.0
      cxn.start_transaction do |tx|
         s = Statement.new(cxn, tx, INSERT_SQL, 3)
         1.upto(20) do |number|
            s.execute_for([number, "Number is #{number}.", decimal, Time.new])
            decimal = decimal + 0.24
         end
         s.close
      end
      
      # Select back the rows inserted and display them
      rows = cxn.execute_immediate(SELECT_SQL)
      rows.each do |row|
         puts "-----"
         puts "Test Id:      #{row['TESTID']}"
         puts "Test Text:    '#{row['TESTTEXT']}'"
         puts "Test Float:   #{row['TESTFLOAT']}"
         puts "Test Created: #{row['CREATED']}"
         puts "-----"
      end
      rows.close
      
      # Drop the table.
      cxn.execute_immediate(DROP_TABLE_SQL)
   end
rescue Excepton => error
   puts error.message
end
