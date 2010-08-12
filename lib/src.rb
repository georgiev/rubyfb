#
# This file is a stand-in that allows for the generation of rdoc documentation
# for the FireRuby extension for the Ruby programming language. The extension
# is coded in C and documented with Doxygen comments, so this file is used to
# generate the native Ruby documentation instead.
#

#
# This module contains all of the classes and definitions relating to the
# FireRuby extension for the Ruby language.
#
module Rubyfb
   #
   # This class provides the exception type used by the FireRuby library.
   #
   class FireRubyException
      #
      # This is the constructor for the FireRubyError class.
      #
      # ==== Parameters
      # message::  A string containing the error message for the object.
      #
      def initialize(message)
      end
      
      
      #
      #  This is the accessor for the error message attribute
      #
      def message
      end
      
      
      #
      # This is the accessor for the SQL code attribute.
      #
      def sql_code
      end
      
      
      #
      # This is the accessor for the database code attribute.
      #
      def db_code
      end
      
      
      #
      # This function generates a simple description string for a FireRubyError
      # object.
      #
      def to_s
      end
   end
   
   
   #
   # This class represents an existing database that can be connected to. It
   # also provides functionality to allow for the creation of new databases.
   #
   class Database
      #
      # This is the constructor for the Database class.
      #
      # ==== Parameters
      # file::      A string containing the database file specifier. This can
      #             include details for a remote server if needed.
      # set::       A string containing the name of the character set to be
      #             used with the database. Defaults to nil.
      #
      def initialize(file, set=nil)
      end
      
      
      #
      # This is the accessor for the database file specification attribute.
      #
      def file
      end
      
      
      #
      # This method attempts to establish a connection to a database. If
      # successful then a Connection instance is returned. If a block is
      # provided to the method then the connection is closed after the
      # block completes. If a block is specified the connection is provided
      # as a parameter to the block.
      #
      # ==== Parameters
      # user::      The user name to be used in making the connection. This
      #             defaults to nil.
      # password::  The password to be used in making the connection. This
      #             defaults to nil.
      # options::   A Hash of connection options. This should be made up of
      #             key/setting pairs where the keys are from the list that
      #             is defined within the Connection class. The settings are
      #             particular to the key, see the documentation for the
      #             Connection class for more details.
      #
      # ==== Exceptions
      # Exception::  Thrown whenever a problem occurs connecting with the
      #              database.
      #
      def connect(user=nil, password=nil, options=nil)
         yield(connection)
      end
      
      
      #
      # This method attempts to drop the database referred to by the details
      # in a Database object.
      #
      # ==== Parameters
      # user::      The user name to be used in dropping the database.
      # password::  The password to be used in dropping the database.
      #
      # ==== Exceptions
      # FireRubyError::  Thrown whenever a problem occurs dropping the database
      #                  instance.
      #
      def drop(user, password)
      end
      
      
      #
      # This method can be used to programmatically created a database file.
      # If successful this method returns a Database object.
      #
      # ==== Parameters
      # file::      A string containing the path and name of the database file
      #             to be created.
      # user::      A string containing the user name that will be used in
      #             creating the file.
      # password::  A string containing the user password that will be used in
      #             creating the file.
      # size::      The page size setting to be used with the new database file.
      #             This should be 1024, 2048, 4096 or 8192. Defaults to 1024.
      # set::       The name of the default character set to be assigned to the
      #             new database file. If this parameter is specifed then the
      #             Database object created by the call will use it to whenever
      #             a connection request is made. Defaults to nil.
      #
      # ==== Exceptions
      # Exception::  Generated whenever an invalid parameter is specified or a
      #              problem occurs creating the database file.
      #
      def Database.create(file, user, password, size=1024, set=nil)
      end


      #
      # This method fetches the name of the character set currently assigned
      # to a Database object. This can return nil to indicate that a explicit
      # character set has not been assigned.
      #
      def character_set
      end


      #
      # This method allows the specification of a database character set that
      # will be used when creating connections to a database. The value set by
      # this method can be overridden by providing an alternative in the connect
      # call. To remove a character set specification from a Database object
      # pass nil to this method.
      #
      # ==== Parameters
      # set::  A string containing the name of the database character set.
      #
      def character_set=(set)
      end
   end
   
   
   #
   # This class represents a connection with a Firebird database.
   #
   class Connection
      # A definition for a connection option. This option should be given a
      # setting of either true or false.
      MARK_DATABASE_DAMAGED       = 17


      # A definition for a connection option. This option should be given a
      # setting of Connection::WRITE_ASYNCHRONOUS or
      # Connection::WRITE_SYNCHRONOUS
      WRITE_POLICY                = 24


      # A definition for a connection option. This option should be given a
      # string setting which should be the name of the character set to be
      # used by the connection.
      CHARACTER_SET               = 48


      # A definition for a connection option. This option should be given a
      # string setting which should be the name of the message file to be used
      # by the connection.
      MESSAGE_FILE                = 47


      # A definition for a connection option. This option should be given a
      # an integer setting. Values between 1 and 255 are valid, with 75 being
      # the default.
      NUMBER_OF_CACHE_BUFFERS     = 5


      # A definition for a connection option. This option should be given a
      # string value which should be the database DBA user name.
      DBA_USER_NAME               = 19


      # A definition for a connection option. This option should be given a
      # string value which should be the ROLE to use when connecting.
      SQL_ROLE_NAME               = 60


      # A definition for a possible setting to accompany the WRITE_POLICY
      # connection setting.
      WRITE_ASYNCHONOUS           = 0


      # A definition for a possible setting to accompany the WRITE_POLICY
      # connection setting.
      WRITE_SYNCHONOUS            = 1
      
      
      #
      # This is the constructor for the Connection class.
      #
      # ==== Parameters
      # database::  A reference to the Database object to be connected to.
      # user::      A reference to the user name to be used in making the
      #             database connection. Defaults to nil.
      # password::  A reference to the user password to be used in making the
      #             connection. Defaults to nil.
      # options::   A Hash containing the options to be applied to the new
      #             connection. The hash will contain key/setting values, with
      #             the keys being based on the option constants defined within
      #             the Connection class. Individual options have differing
      #             value requirements. See the option documentation entries
      #             for further details. Defaults to nil.
      #
      # ==== Exceptions
      # Exception::  Generated whenever an invalid database is specified to
      #              the method or an issue occurs establishing the database
      #              connection.
      #
      def initialize(database, user, password, options)
      end
      
      
      #
      # This method is used to determine whether a Connection object represents
      # an active database connection.
      #
      def open?
      end
      
      
      #
      # This method is used to determine whether a Connection object represents
      # an inactive database connection.
      #
      def closed?
      end
      
      
      #
      # This method detaches a Connection object from a database. The object
      # may not be used for database functionality following a successful call
      # to this method. The close method will fail if there are outstanding
      # transactions for a connection.
      #
      # ==== Exceptions
      # Exception::  Generated whenever the connection has at least one open
      #              transaction or an error occurs closing the connection.
      #
      def close
      end
      
      
      #
      # This is the accessor method for the database attribute.
      #
      def database
      end
      
      
      #
      # This method retrieves the user name that was used in creating a
      # Connection object.
      #
      def user
      end
      
      
      #
      # This method generates a simple descriptive string for a Connection
      # object.
      #
      def to_s
      end
      
      
      #
      # This method starts a new transaction against a connection. A successful
      # call to this method returns a Transaction object. The transaction that
      # is started relates to the Connection it was called upon only. To start
      # a transaction that covers multiple connections use the Transaction
      # class. This method accepts a block, taking a single parameter which
      # will be the transaction created. This transaction is committed if the
      # block completes normally or rolls back if an exception is thrown from
      # the block.
      #
      # ==== Exceptions
      # Exception::  Thrown whenever a problem occurs starting the transaction.
      #
      def start_transaction
         yield transaction
      end
      
      
      #
      # This function executes a SQL statement against a connection. If the
      # statement represented a SQL query then a ResultSet object is returned.
      # If the statement was a non-query SQL statement then an Integer is
      # returned indicating the number of rows affected by the statement. For
      # all other types of statement the method returns nil. The method also
      # accepts a block that takes a single parameter. This block will be
      # executed once for each row in any result set generated.
      #
      # ==== Parameters
      # sql::          The SQL statement to be executed.
      # transaction::  The transaction to execute the SQL statement within.
      #
      # ==== Exceptions
      # Exception::  Generated if an error occurs executing the SQL statement.
      #
      def execute(sql, transaction)
         yield(row)
      end
      
      
      #
      # This function executes a SQL statement against a connection. This 
      # differs from the execute method in that an anonymous transaction is
      # used in executing the statement. The output from this method is the
      # same as for the execute method. The method also accepts a block that
      # takes a single parameter. This block will be executed once for each
      # row in any result set generated.
      #
      # ==== Parameters
      # sql::  The SQL statement to be executed.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs executing the SQL
      #              statement.
      #
      def execute_immediate(sql)
         yield(row)
      end
   end
   
   
   #
   # This class represents a Firebird database transaction. There may be
   # multiple transaction outstanding against a connection at any one time.
   #
   class Transaction
      TPB_VERSION_1          = 1
      TPB_VERSION_3          = 3
      TPB_CONSISTENCY        = 1
      TPB_CONCURRENCY        = 2
      TPB_SHARED             = 3
      TPB_PROTECTED          = 4
      TPB_EXCLUSIVE          = 5
      TPB_WAIT               = 6
      TPB_NO_WAIT            = 7
      TPB_READ               = 8
      TPB_WRITE              = 9
      TPB_LOCK_READ          = 10
      TPB_LOCK_WRITE         = 11
      TPB_VERB_TIME          = 12
      TPB_COMMIT_TIME        = 13
      TPB_IGNORE_LIMBO       = 14
      TPB_READ_COMMITTED     = 15
      TPB_AUTO_COMMIT        = 16
      TPB_REC_VERSION        = 17
      TPB_NO_REC_VERSION     = 18
      TPB_RESTART_REQUESTS   = 19
      # Transaction parameter buffer value constants.
      TPB_NO_AUTO_UNDO       = 20

      
      #
      # This is the constructor for the Transaction class.
      #
      # ==== Parameters
      # connections::  Either a single instance of the Connection class or
      #                an array of Connection instances to specify a
      #                multi-database transaction.
      #
      # ==== Exceptions
      # Exception::  Generated whenever the method is passed an invalid
      #              parameter or a problem occurs creating the transaction.
      #
      def initialize(connections)
      end
      
      
      #
      # This method is used to determine whether a Transaction object is still
      # valid for use (i.e. commit or rollback has not been called for the
      # Transaction).
      #
      def active?
      end
      
      
      #
      # This is the accessor for the connections attribute. This method returns
      # an array of the connections that the transaction applies to.
      #
      def connections
      end
      
      
      #
      # This method is used to determine whether a given Transaction applies to
      # a specified Connection.
      #
      # ==== Parameters
      # connection::  A reference to the Connection object to perform the test
      #               for.
      #
      def for_connection?(connection)
      end

      
      #
      # This method commits the details outstanding against a Transaction
      # object. The Transaction object may not be reused after a successful
      # call to this method.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs committing the details
      #              of the transaction.
      #
      def commit
      end

      
      #
      # This method rolls back the details outstanding against a Transaction
      # object. The Transaction object may not be reused after a successful
      # call to this method.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs rolling back the
      #              details of the transaction.
      #
      def rollback
      end
      
      
      #
      # This method executes a SQL statement using a Transaction object. This
      # method will only work whenever a Transaction object applies to a
      # single Connection as it would otherwise be impossible to determine
      # which connection to execute against. If the statement executed was a
      # SQL query then the method returns a ResultSet object. For non-query SQL
      # statements (insert, update or delete) the method returns an Integer that
      # contains the number of rows affected by the statement. For all other
      # statements the method returns nil. The method  also accepts a block that
      # takes a single parameter. If the SQL statement was a query the block
      # will be invoked and passed each row retrieved.
      #
      # ==== Parameters
      # sql::  A string containing the SQL statement to be executed.
      #
      # ==== Exceptions
      # Exception::  Generated whenever the Transaction object represents more
      #              than one connection or a problem occurs executing the SQL
      #              statement.
      #
      def execute(sql)
         yield(row)
      end
      
      
      #
      # This method allows for the creation of a Transaction object with
      # non-standard settings.
      #
      # ==== Parameters
      # connections::  Either a single Connection object or an array of
      #                Connection objects that the new Transaction will
      #                be associated with.
      # parameters::   An array of the parameters to be used in creating
      #                the new constants. Populate this from the TPB
      #                constants defined within the class.
      #
      # ==== Exceptions
      # FireRubyError::  Generated whenever a problem occurs creating the
      #                  transaction.
      #
      def Transaction.create(connections, parameters)
      end
   end
   
   
   #
   # This class  represents a prepared SQL statement that may be executed more
   # than once.
   #
   class Statement
      # A definition for a SQL statement type constant.
      SELECT_STATEMENT            = 1
      
      # A definition for a SQL statement type constant.
      INSERT_STATEMENT            = 2
      
      # A definition for a SQL statement type constant.
      UPDATE_STATEMENT            = 3
      
      # A definition for a SQL statement type constant.
      DELETE_STATEMENT            = 4
      
      # A definition for a SQL statement type constant.
      DDL_STATEMENT               = 5
      
      # A definition for a SQL statement type constant.
      GET_SEGMENT_STATEMENT       = 6
      
      # A definition for a SQL statement type constant.
      PUT_SEGMENT_STATEMENT       = 7
      
      # A definition for a SQL statement type constant.
      EXECUTE_PROCEDURE_STATEMENT = 8
      
      # A definition for a SQL statement type constant.
      START_TRANSACTION_STATEMENT = 9
      
      # A definition for a SQL statement type constant.
      COMMIT_STATEMENT            = 10
      
      # A definition for a SQL statement type constant.
      ROLLBACK_STATEMENT          = 11
      
      # A definition for a SQL statement type constant.
      SELECT_FOR_UPDATE_STATEMENT = 12
      
      # A definition for a SQL statement type constant.
      SET_GENERATOR_STATEMENT     = 13
      
      # A definition for a SQL statement type constant.
      SAVE_POINT_STATEMENT        = 14
      
      #
      # This is the constructor for the Statement class.
      #
      # ==== Parameters
      # connection::   The Connection object that the SQL statement will be
      #                executed through.
      # transaction::  The Transaction object that the SQL statement will be
      #                executed under.
      # sql::          The SQL statement to be prepared for execution.
      # dialect::      The Firebird dialect to be used in preparing the SQL
      #                statement.
      #
      def initialize(connection, transaction, sql, dialect)
      end
      
      
      #
      # This is the accessor for the connection attribute.
      #
      def connection
      end
      
      
      #
      # This is the accessor for the transaction attribute.
      #
      def transaction
      end
      
      
      #
      # This is the accessor for the SQL statement attribute.
      #
      def sql
      end
      
      
      #
      # This is the accessor for the dialect attribute.
      #
      def dialect
      end
      
      
      #
      # This method is used to determine the type of a SQL statement. The method
      # will return one of the constant SQL types defined within the class.
      #
      def type
      end
      
      
      #
      # This method fetches a count of the number of dynamic parameters for
      # a statement object (i.e. the number of parameters that must be provided
      # with values before the SQL statement can be executed).
      #
      def parameter_count
      end
      
      
      #
      # This method executes the SQL statement within a Statement object. This
      # method returns a ResultSet object if the statement executed was a SQL
      # query. For non-query SQL statements (insert, update or delete) it
      # returns an Integer indicating the number of affected rows. For all other
      # statements the method returns nil. This method accepts a block taking a
      # single parameter. If this block is provided and the statement is a query
      # then the rows returned by the query will be passed, one at a time, to
      # the block.
      #
      # ==== Exception
      # Exception::  Generated if the Statement object actual requires some
      #              parameters or a problem occurs executing the SQL statement.
      #
      def execute
         yield row
      end
      
      
      #
      # This method executes the SQL statement within a Statement object and
      # passes it a set of parameters. Parameterized statements use question
      # marks as place holders for values that may change between calls to
      # execute the statement. This method returns a ResultSet object if the
      # statement executed was a SQL query. If the statement was a non-query SQL
      # statement (insert, update or delete) then the method returns a count of
      # the number of rows affected. For all other types of statement the method
      # returns nil. This method accepts a block taking a single parameter. If
      # this block is provided and the statement is a query then the rows
      # returned by the query will be passed, one at a time, to the block.
      #
      # ==== Parameters
      # parameters::  An array of the parameters for the statement. An effort
      #               will be made to convert the values passed in to the
      #               appropriate types but no guarantees are made (especially
      #               in the case of text fields, which will simply use to_s
      #               if the object passed is not a String).
      #
      # ==== Exception
      # Exception::  Generated whenever a problem occurs translating one of the
      #              input parameters or executing the SQL statement.
      #
      def execute_for(parameters)
         yield row
      end
      
      
      #
      # This method releases the database resources associated with a Statement
      # object and should be explicitly called when a Statement object is of
      # no further use.
      #
      # ==== Exceptions
      # FireRubyError::  Generated whenever a problem occurs closing the
      #                  statement object.
      #
      def close
      end
   end
   
   
   #
   # This class represents the results of a SQL query executed against a
   # database. The viable lifespan of a ResultSet object is limited by the
   # transaction that was used in it's creation. Once this transaction has
   # been committed or rolled back all related ResultSet object are invalidated
   # and can no longer be used.
   #
   class ResultSet
      include Enumerable
      
      #
      # This is the constructor for the ResultSet object.
      #
      # ==== Parameters
      # connection::   A reference to the Connection object that will be used
      #                to execute the SQL query.
      # transaction::  A reference to the Transaction object that will be used
      #                in executing the SQL query.
      # sql::          A reference to a String containing the SQL query that
      #                will be executed.
      # dialect::      A reference to an integer containing the Firebird dialect
      #                to be used in executing the SQL statement.
      #
      # ==== Exceptions
      # FireRubyException::  Generated whenever a non-query SQL statement is
      #                      specified, an invalid connection or transaction is
      #                      provided or a problem occurs executing the SQL
      #                      against the database.
      #
      def initialize(connection, transaction, sql, dialect)
      end
      
      
      #
      # This is the accessor for the connection attribute.
      #
      def connection
      end
      
      
      #
      # This is the accessor for the transaction attribute.
      #
      def transaction
      end
      
      
      #
      # This is the accessor for the sql attribute.
      #
      def sql
      end
      
      
      #
      # This is the accessor for the dialect attribute.
      #
      def dialect
      end
      
      
      #
      # This method fetches a count of the number of columns in a row of data
      # that the ResultSet can fetch.
      #
      def column_count
      end
      
      
      #
      # This method fetches the name associated with a specified column for a
      # ResultSet object.
      #
      # ==== Parameters
      # column::  A reference to the column number to fetch the details for.
      #           Column numbers start at zero.
      #
      def column_name(column)
      end
      
      
      #
      # This method fetches the alias associated with a specified column for a
      # ResultSet object.
      #
      # ==== Parameters
      # column::  A reference to the column number to fetch the details for.
      #           Column numbers start at zero.
      #
      def column_alias(column)
      end
      
      
      #
      # This method fetches the table name associated with a specified column
      # for a ResultSet object.
      #
      # ==== Parameters
      # column::  A reference to the column number to fetch the details for.
      #           Column numbers start at zero.
      #
      def column_table(column)
      end
      
      
      #
      # This method fetches a single rows worth of data from the ResultSet
      # object. If the set contains more rows then an array containing the
      # row data will be retrieved. If the ResultSet is exhausted (i.e. all
      # rows have been fetched) then nil is returned. Translation of the row
      # data into an appropriate Ruby type is performed on the row data that
      # is extracted.
      #
      def fetch
      end
      
      
      #
      # This method is used to determine if all of the rows have been retrieved
      # from a ResultSet object. This method will always return false until
      # the fetch method has been called at least once so it cannot be used to
      # detect a result set that returns no rows.
      #
      def exhausted?
      end
      
      
      #
      # This method fetches a count of the total number of rows retrieved
      # from a result set.
      #
      def row_count
      end
      
      
      #
      # This method provides an iterator for the (remaining) rows contained in
      # a ResultSet object.
      #
      # ==== Parameters
      # block::  A block that takes a single parameter. This will be called for
      #          and passed each remaining row (as per the fetch method) from
      #          the ResultSet.
      #
      def each(&block)
      end
      
      
      #
      # This method releases the database resources associated with a ResultSet
      # object and should be explicitly called when a ResultSet object is of
      # no further use. The method is implicitly called if the rows available
      # from a ResultSet are exhausted but calling this method at that time
      # will not cause an error.
      #
      # ==== Exceptions
      # FireRubyError::  Generated whenever a problem occurs closing the result
      #                  set object.
      #
      def close
      end
      
      #
      # This method retrieves the base SQL type for a column of data within a
      # ResultSet object. The method returns one of the base types defined in
      # the SQLType class but does not return an actual SQLType object.
      #
      # ==== Parameters
      # index::  The offset from the ResultSet first column of the column to
      #          return the type information for.
      #
      def get_base_type(index)
      end
   end
   
   
   #
   # This class models a row of data fetched as part of a SQL query.
   #
   class Row
      include Enumerable
      
      #
      # This is the constructor for the Row class. This method shouldn't really
      # be used as Row objects are automatically created by ResultSets.
      #
      # ==== Parameters
      # results::  The ResultSet object that the row relates to.
      # data::     An array containing the row data values.
      # number::   The row number for the new row.
      #
      def initialize(results, data, number)
      end
      
      #
      # This is the accessor for the row number attribute. This will generally
      # reflect the order the row was fetched from the result set in, with 1
      # being the first row retrieved.
      #
      def number
      end
      
      
      #
      # This method fetches a count of the number of columns of data that are
      # available from a row.
      #
      def column_count
      end
      
      
      #
      # This method fetches the name of a column within a row of data.
      #
      # ==== Parameters
      # index::  The index of the column to fetch the name for. The first
      #          column in the row is at offset zero.
      #
      def column_name(index)
      end
      
      
      #
      # This method fetches the alias of a column within a row of data.
      #
      # ==== Parameters
      # index::  The index of the column to fetch the alias for. The first
      #          column in the row is at offset zero.
      #
      def column_alias(index)
      end
      
      
      #
      # This method fetches the value associated with a column within a Row
      # object.
      #
      # ==== Parameters
      # index::  Either the offset of the column to retrieve the value of or
      #          the alias of the column to retrieve the value of (column alias
      #          comparisons are case sensitive).
      #
      def [](index)
      end
      
      
      #
      # This method iterates over the contents of a Row object. The block
      # specified for the method should accept two parameters; one for the
      # column alias and one for the column value.
      #
      def each
         yield column, vlaue
      end
      
      
      #
      # This method iterates over the column names for a Row class.
      #
      def each_key
         yield column
      end
      
      
      #
      # This method iterators over the column values for a Row class.
      #
      def each_value
         yield value
      end
      
      
      #
      # An implementation of the Hash#fetch method for the Row class. The
      # method accepts a block but this should not be specified if a value
      # for the alternative parameter is specified.
      #
      # ==== Parameters
      # key::          A string containing the column alias to retrieve.
      # alternative::  A reference to the alternative value to be returned if
      #                the keyed value is not found. Defaults to nil.
      #
      def fetch(key, alternative=nil)
         yield key
      end
      
      
      #
      # This method is used to determine whether a Row object contains a given
      # column alias.
      #
      # ==== Parameters
      # name::  A String containing the column name to check for.
      #
      def has_key?(name)
      end
      
      
      #
      # This method is used to determine whether a Row object contains a given
      # column name.
      #
      # ==== Parameters
      # name::  A String containing the column name to check for.
      #
      def has_column?(name)
      end
      
      
      #
      # This method is used to determine whether a Row object contains a given
      # column alias.
      #
      # ==== Parameters
      # name::  A String containing the column alias to check for.
      #
      def has_alias?(name)
      end
      
      
      #
      # This method is used to determine whether a Row object contains a given
      # column value.
      #
      # ==== Parameters
      # value::  A reference to an object value to be checked for.
      #
      def has_value?(value)
      end
      
      
      #
      # This method retrieves an array of column aliases for a Row object.
      #
      def keys
      end
      
      
      #
      # This method retrieves an array of column names for a Row object.
      #
      def names
      end
      
      
      #
      # This method retrieves an array of column aliases for a Row object.
      #
      def aliases
      end
      
      
      #
      # This method retrieves an array of column values for a Row object.
      #
      def values
      end
      
      
      #
      # This method returns an array of the Row elements for which the specified
      # block returns true.
      #
      def select
         yield column, value
      end
      
      
      #
      # This method retrieves an Array containing the values from a Row object.
      # Each value is represented as an Array containing a column name and the
      # associated column value.
      #
      def to_a
      end
      
      
      #
      # This method retrieves a Hash created from a Row objects values. The Row
      # objects column names will be used as a key on to the column values.
      #
      def to_hash
      end
      
      
      #
      # This method returns an array of column values based on a list of column
      # aliases.
      #
      # ==== Parameters
      # names::  One or more Strings containing the names of the columns to
      #          retrieve values for.
      #
      def values_at(*names)
      end
      
      #
      # This method retrieves the base SQL type for a column of data within a
      # Row object. The method returns one of the base types defined in the
      # SQLType class but does not return an actual SQLType object.
      #
      # ==== Parameters
      # index::  The offset from the Row first column of the column to return
      #          the type information for.
      #
      def get_base_type(index)
      end
   end
   
   #
   # This class represents Blob data fetched from the database. The class defers
   # the actual loading of the blob until requested. The class is somewhat basic
   # and maybe expanded upon in later releases.
   #
   class Blob
      #
      # This is the constructor for the Blob class. This shouldn't really be
      # used outside of the FireRuby library.
      #
      def initialize
      end
      
      
      #
      # This method loads the entire data set for a blob as a string.
      #
      def to_s
      end
      
      
      #
      # This method closes a blob, freeing any resources associated with it.
      #
      def close
      end
      
      
      #
      # This method loads the segments of a blob one after another. The blob
      # segments are passed as strings to the block passed to the method.
      #
      def each
         yield segment
      end
   end
   
   
   #
   # This class represents a Firebird generator entity.
   #
   class Generator
      #
      # This is the constructor for the Generator class. Note, this method
      # assumes that the named generator already exists. If it doesn't then
      # the object will be constructed but will fail during use.
      #
      # ==== Parameters
      # name::        A string containing the generator name.
      # connection::  A reference to the Connection object that will be used
      #               to access the generator.
      #
      def initialize(name, connection)
      end
      
      
      #
      # This is the accessor for the name attribute.
      #
      def name
      end
      
      
      #
      # This is the accessor for the connection attribute.
      #
      def connection
      end
      
      
      #
      # This method fetches the last value generator from a generator.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs accessing the
      #              database generator.
      #
      def last
      end
      
      
      #
      # This method drops a generator from the database. After a successful
      # call to this method the Generator object may not be used to obtain
      # values unless it is recreated.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs dropping the generator
      #              from the database.
      #
      def drop
      end
      
      
      #
      # This method fetches the next value, depending on a specified increment,
      # from a generator.
      #
      # ==== Parameters
      # step::  The step interval to be applied to the generator to obtain the
      #         next value.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs accessing the
      #              database generator.
      #
      def next(step)
      end
      
      
      #
      # This method is used to determine whether a named generator exists
      # within a database.
      #
      # ==== Parameters
      # name::        A string containing the generator name to check for.
      # connection::  A reference to the Connection object to be used in
      #               performing the check.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs determining the
      #              existence of the generator.
      #
      def Generator.exists?(name, connection)
      end
      
      
      #
      # This method creates a new generator within a database. This method
      # returns a Generator object is successful.
      #
      # ==== Parameters
      # name::        A string containing the name for the new generator.
      # connection::  A reference to the Connection object that will be used to
      #               create the generator.
      #
      # ==== Exceptions
      # Exception::  Generated whenever a problem occurs creating the new
      #              generator in the database.
      #
      def Generator.create(name, connection)
      end
   end
   
   
   #
   # This class represents a connection to the service manager for a Firebird
   # database server instance. NOTE: This class does not currently work on the
   # Mac OS X platform.
   #
   class ServiceManager
      #
      # This is the constructor for the ServiceManager class.
      #
      # ==== Parameters
      # host::  The name of the host supporting the service manager to be
      #         connected with.
      #
      def initialize(host)
      end
      
      
      #
      # This method attaches a ServiceManager object to its host service. The
      # user name used to connect with can affect which services can be accessed
      # on the server.
      #
      # ==== Parameters
      # user::      A string containing the user name to connect with.
      # password::  A string containing the user password to connect with.
      #
      def connect(user, password)
      end
      
      
      #
      # This method disconnects a previously connected ServiceManager object.
      #
      def disconnect
      end
      
      
      #
      # This method is used to determine whether a ServiceManager object has
      # been connected.
      #
      def connected?
      end
      
      
      #
      # This method is used to batch execute a collection of task objects.
      #
      # ==== Parameters
      # tasks::  One or more task objects to be executed by the service manager.
      #
      # ==== Exceptions
      # FireRubyException::  Generated whenever this method is called on a
      #                      disconnected service manager or is a problem
      #                      occurs executing one of the tasks.
      #
      def execute(*tasks)
      end
   end
   
   
   #
   # This class represents a service manager task to add a new user to a
   # database instance. NOTE: This class does not currently work on the
   # Mac OS X platform.
   #
   class AddUser
      # Attribute accessor.
      attr_reader :user_name, :password, :first_name, :middle_name, :last_name
      
      # Attribute mutator.
      attr_writer :user_name, :password, :first_name, :middle_name, :last_name
      
      #
      # This is a constructor for the AddUser class.
      #
      # ==== Parameters
      # user_name::    A String containing the user name to be assigned to the
      #                new user.
      # password::     A String containing the password to be assigned to the
      #                new user.
      # first_name::   A String containing the first name to be associated with
      #                the new user. Defaults to nil.
      # middle_name::  A String containing the middle name to be associated
      #                with the new user. Defaults to nil.
      # last_name::    A String containing the last name to be associated with
      #                the new user. Defaults to nil.
      #
      def initialize(user_name, password, firsts_name=nil, middle_name=nil,
                     last_name=nil)
      end
      
      
      #
      # This method executes the add user task against a service manager.
      #
      # ==== Parameters
      # manager::  A reference to the ServiceManager object to execute the task
      #            on.
      #
      # ==== Exceptions
      # FireRubyException::  Generated whenever a disconnected service manager
      #                      is specified or an error occurs executing the
      #                      task.
      #
      def execute(manager)
      end
   end
   
   
   #
   # This class represents a service manager task to remove an existing user
   # from a database instance. NOTE: This class does not currently work on the
   # Mac OS X platform.
   #
   class RemoveUser
      # Attribute accessor.
      attr_reader :user_name
      
      # Attribute mutator.
      attr_writer :user_name
      
      #
      # This is a constructor for the RemoveUser class.
      #
      # ==== Parameters
      # user_name::    A String containing the user name to be assigned to the
      #                new user.
      #
      def initialize(user_name)
      end
      
      
      #
      # This method executes the remove user task against a service manager.
      #
      # ==== Parameters
      # manager::  A reference to the ServiceManager object to execute the task
      #            on.
      #
      # ==== Exceptions
      # FireRubyException::  Generated whenever a disconnected service manager
      #                      is specified or an error occurs executing the
      #                      task.
      #
      def execute(manager)
      end
   end
   
   
   #
   # This class represents a service manager task to backup an existing database
   # on the Firebird server. NOTE: This class does not currently work on the
   # Mac OS X platform.
   #
   class Backup
      # Attribute accessor.
      attr_reader :backup_file, :database
      
      # Attribute mutator.
      attr_writer :backup_file, :database
      
      #
      # This is the constructor for the Backup class.
      #
      # ==== Parameters
      # database::  A String or File giving the path and name (relative to the
      #             database server) of the main database file for the database
      #             to be backed up.
      # file::      A String or File giving the path and name (relative to the
      #             database server) of the back up file to be generated.
      #
      def initialize(database, file)
      end
      
      
      #
      # This method fetches the blocking factor to be used in generating the
      # back up. This will return nil until it has been explicitly set.
      #
      def blocking_factor
      end
      
      
      #
      # This method sets the blocking factor to be used in generating the
      # back up.
      #
      # ==== Parameters
      # size::  A reference to an integer containing the new back up blocking
      #         factor setting.
      #
      def blocking_factor=(size)
      end
      
      
      #
      # This method fetches the ignore checksums setting for a Backup object.
      #
      def ignore_checksums
      end
      
      
      #
      # This method is used to set the indicator for whether checksum values
      # should be ignored in performing a backup.
      #
      # ==== Parameters
      # setting::  True to ignore checksums, false otherwise.
      #
      def ignore_checksums=(setting)
      end
      
      
      #
      # This method fetches the ignore limbo setting for a Backup object.
      #
      def ignore_limbo
      end
      
      
      #
      # This method is used to set the indicator for whether limbo transactions
      # should be ignored in performing a backup.
      #
      # ==== Parameters
      # setting::  True to ignore limbo transactions, false otherwise.
      #
      def ignore_limbo=(setting)
      end
      
      
      #
      # This method fetches the metadata only setting for a Backup object.
      #
      def metadata_only
      end
      
      
      #
      # This method is used to set the indicator for whether a backup stores
      # only the database metadata.
      #
      # ==== Parameters
      # setting::  True to store only metadata, false otherwise.
      #
      def metadata_only=(setting)
      end
      
      
      #
      # This method fetches the garbage collect setting for a Backup object.
      #
      def garbage_collect
      end
      
      
      #
      # This method is used to set the indicator for whether the backup will
      # undertake garbage collection.
      #
      # ==== Parameters
      # setting::  True to perform garbage collection, false otherwise.
      #
      def garbage_collect=(setting)
      end
      
      
      #
      # This method fetches the non-transportable setting for a Backup object.
      #
      def non_transportable
      end
      
      
      #
      # This method is used to set the indicator for whether backup generated
      # by the task will be platform specific.
      #
      # ==== Parameters
      # setting::  True to generate a platform specific backup, false otherwise.
      #
      def non_transportable=(setting)
      end
      
      
      #
      # This method fetches the convert tables setting for a Backup object.
      #
      def convert_tables
      end
      
      
      #
      # This method is used to set the indicator for whether external tables
      # will be converted to internal tables as part of the backup.
      #
      # ==== Parameters
      # setting::  True to convert external tables, false otherwise.
      #
      def convert_tables=(setting)
      end
      
      
      #
      # This method is used to execute a backup task against a service manager.
      #
      # ==== Parameters
      # manager::  A reference to the service manager to execute the backup
      #            task against.
      #
      # ==== Exceptions
      # FireRubyException::  Generated whenever a disconnected service manager
      #                      is specified or a problem occurs executing the
      #                      task.
      #
      def execute(manager)
      end
      
      
      #
      # This method fetches the log value for a Backup task. This value will
      # always be nil until the task has been executed. After a successful
      # execution the log value should contain output from the backup task
      # generated on the server.
      #
      def log
      end
   end
   
   
   #
   # This class represents a service manager task to restore a previously
   # created database backup on the Firebird server. NOTE: This class does not
   # currently work on the Mac OS X platform.
   #
   class Restore
      # Attribute accessor.
      attr_reader :backup_file, :database
      
      # Attribute mutator.
      attr_writer :backup_file, :database
      
      # Access mode constant definition.
      ACCESS_READ_ONLY       = 39
      
      # Access mode constant definition.
      ACCESS_READ_WRITE      = 40
      
      # Restore mode constant definition.
      MODE_CREATE            = 0x1000
      
      # Restore mode constant definition.
      MODE_REPLACE           = 0x2000
      
      #
      # This is the constructor for the Restore class.
      #
      # ==== Parameters
      # file::      A String or File containing the path and name (relative to
      #             the server) of the backup file to be used in the restore.
      # database::  A String or File containing the path and name (relative to
      #             the server) of the database file to be restored.
      #
      def initialize(file, database)
      end
      
      
      #
      # This method retrieves the cache buffers setting for a Restore object.
      # This will be nil until a value is actual set.
      #
      def cache_buffers
      end
      
      
      #
      # This method updates the cache buffers setting for a Restore object.
      #
      # ==== Parameters
      # setting::  The new value for the object setting. Should be an integer.
      #
      def cache_buffers=(setting)
      end
      
      
      #
      # This method retrieves the page size setting for a Restore object.
      # This will be nil until a value is actual set.
      #
      def page_size
      end
      
      
      #
      # This method updates the page size setting for a Restore object.
      #
      # ==== Parameters
      # setting::  The new value for the object setting. Should be an integer.
      #
      def page_size=(setting)
      end
      
      
      #
      # This method retrieves the access mode setting for a Restore object.
      # This will be nil until a value is actual set.
      #
      def access_mode
      end
      
      
      #
      # This method updates the access mode setting for a Restore object.
      #
      # ==== Parameters
      # setting::  The new value for the object setting. This should be one
      #            of Restore::ACCESS_READ_ONLY or Restore::ACCESS_READ_WRITE.
      #
      def access_mode=(setting)
      end
      
      
      #
      # This method retrieves the build indices setting for a Restore object.
      #
      def build_indices
      end
      
      
      #
      # This method updates the build indices setting for a Restore object.
      # This value affects whether the various indexes for a database are
      # restored with the restore task.
      #
      # ==== Parameters
      # setting::  True to rebuild the database indices, false otherwise.
      #
      def build_indices=(setting)
      end
      
      
      #
      # This method retrieves the no shadows setting for a Restore object.
      #
      def no_shadows
      end
      
      
      #
      # This method updates the no shadows setting for a Restore object.
      # This value affects whether shadow databases are recreated as part of a
      # restore.
      #
      # ==== Parameters
      # setting::  True to recreate shadow files, false otherwise.
      #
      def no_shadows=(setting)
      end
      
      
      #
      # This method retrieves the validity checks setting for a Restore object.
      #
      def check_validity
      end
      
      
      #
      # This method updates the validity checks setting for a Restore object.
      # This value affects whether the restore performs validity checks on the
      # database as it is restored.
      #
      # ==== Parameters
      # setting::  True to perform validity checks, false otherwise.
      #
      def check_validity=(setting)
      end
      
      
      #
      # This method retrieves the commit tables setting for a Restore object.
      #
      def commit_tables
      end
      
      
      #
      # This method updates the commit tables setting for a Restore object.
      # This value affects whether the restore commits tables as they are
      # restored.
      #
      # ==== Parameters
      # setting::  True to commit tables as they are restored, false otherwise.
      #
      def commit_tables=(setting)
      end
      
      
      #
      # This method retrieves the restore mode setting for a Restore object.
      #
      def restore_mode
      end
      
      
      #
      # This method updates the restore mode setting for a Restore object.
      # This value affects whether the restore will overwrite an existing
      # database.
      #
      # ==== Parameters
      # setting::  Either Restore::MODE_CREATE (default) or
      #            Restore::MODE_REPLACE.
      #
      def restore_mode=(setting)
      end
      
      
      #
      # This method retrieves the use all space setting for a Restore object.
      #
      def use_all_space
      end
      
      
      #
      # This method updates the use all space setting for a Restore object.
      # This value affects whether restore leaves space within the database
      # file for expansion. This can be switched on for read only databases.
      #
      # ==== Parameters
      # setting::  True leave no default expansion space within the restored
      #            database file, false otherwise.
      #
      def use_all_space=(setting)
      end
      
      
      #
      # This method is used to execute a restore task against a service manager.
      #
      # ==== Parameters
      # manager::  A reference to the service manager to execute the restore
      #            task against.
      #
      # ==== Exceptions
      # FireRubyException::  Generated whenever a disconnected service manager
      #                      is specified or a problem occurs executing the
      #                      task.
      #
      def execute(manager)
      end
      
      
      #
      # This method fetches the log value for a Restore task. This value will
      # always be nil until the task has been executed. After a successful
      # execution the log value should contain output from the restore task
      # generated on the server.
      #
      def log
      end
   end
end
