#!/usr/bin/env ruby

#-------------------------------------------------------------------------------
# Old Unit Test Suite
#
# This code has been dropped for two reasons. First, adding new tests requires
# that this code be updated. Second, running the tests in a single Ruby
# interpreter seems to cause issues as the tests create, use and drop a lot of
# database files and this seems to cause intermittent problems with one or more
# of the test scripts that doesn't occur when the script is run on its own.
# Changing the number of scripts run also seemed to cause the problems to go
# away. It didn't seem to matter which scripts where left out which leads me
# to believe that the problem is related to timing issues.
#
# The new unit test suite, below, searches the directory for unit test files
# and executes each in their own interpreter. I have left this code here for
# reference purposes.
#-------------------------------------------------------------------------------
#require 'DatabaseTest'
#require 'ConnectionTest'
#require 'TransactionTest'
#require 'StatementTest'
#require 'ResultSetTest'
#require 'RowCountTest'
#require 'RowTest'
#require 'GeneratorTest'
#require 'DDLTest'
#require 'SQLTest'
#require 'ServiceManagerTest'
#require 'CharacterSetTest'
#require 'KeyTest'
#require 'TypeTest'
#require 'SQLTypeTest'
#if PLATFORM.include?('powerpc-darwin') == false
#   require 'BackupRestoreTest'
#   require 'AddRemoveUserTest'
#end
#-------------------------------------------------------------------------------
SPECIALS = ['AddRemoveUserTest',
            'BackupRestoreTest',
            'ServiceManagerTest',
            'RoleTest']
begin
   files = Dir.entries(".")
   files.reject! do |name|
                    ['.', '..', 'UnitTest.rb'].include?(name) or
                    name[-7,7] != 'Test.rb'
                 end
   files.each do |name|
      execute = true
      if SPECIALS.include?(name)
         execute = !(PLATFORM.include?('powerpc-darwin'))
      end
      
      if execute
         system("ruby #{name}")
         
         if $? != 0
            raise StandardError.new("Error executing '#{name}'. Testing terminated.")
         end
      end
   end
rescue => error
   puts "\n\nERROR: #{error.message}"
end
