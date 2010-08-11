#!/usr/bin/env ruby

# Add extra search paths.
basedir = File::dirname(Dir.getwd)
$:.push(basedir)
$:.push("#{basedir}#{File::SEPARATOR}test")
$:.push("#{basedir}#{File::SEPARATOR}lib")

DB_USER_NAME = 'sysdba'
DB_PASSWORD  = 'masterkey'
TEST_LOGGING = false
