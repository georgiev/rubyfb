#!/usr/bin/env ruby

require 'fireruby'

include FireRuby

db = Database.new('sysdba', 'masterkey', './test.fdb')
c  = db.connect
t  = c.start_transaction
ARGV.each do |table|
   count = 0
   s = Statement.new(c, t, "SELECT * FROM #{table}", 3)
   r = ResultSet.new(s)
   r.each do |row|
      row.each_index {|index| row[index] = "NULL" if row[index] == nil}
      puts row.join(", ")
   end
end
