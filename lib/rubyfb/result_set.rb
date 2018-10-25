module Rubyfb
  class ResultSet
    include Enumerable
    attr_reader :statement, :transaction, :row_count
    
    def initialize(statement, transaction)
      @statement = statement
      @transaction = transaction
      @active = true
      @row_count = 0
      @manage_statement = false
      @manage_transaction = false
    end
    
    def initialize_copy(o)
      raise FireRubyException.new("Object cloning is not supported");
    end
    
    def each
      while row = fetch
        yield row
      end
    ensure
      close
    end
    
    def fetch
      row = nil
      case statement.fetch
        when Statement::FETCH_MORE
          @row_count += 1
          row = Row.new(statement.metadata, statement.current_row(transaction), @row_count)
        when Statement::FETCH_COMPLETED
          close
        when Statement::FETCH_ONE
          @row_count = 1
          row = Row.new(statement.metadata, statement.current_row(transaction), @row_count)
          @active = false
        else
          raise FireRubyException.new("Error fetching query row.");
      end if active?
      
      return row
    end
    
    def close
      return unless active?
      
      @active = false
      statement.close_cursor
      if @manage_statement && statement.prepared?
        statement.close
      end
      if @manage_transaction && transaction.active?
        transaction.commit
      end
    end
    
    def connection
      statement.connection
    end
    
    def sql
      statement.sql
    end
    
    def dialect
      statement.dialect
    end
    
    def column_name(index)
      with_metadata(index){ |m| m.name }
    end
    
    def column_alias(index)
      with_metadata(index){ |m| m.alias }
    end
    
    def column_scale(index)
      with_metadata(index){ |m| m.scale }
    end
    
    def column_table(index)
      with_metadata(index){ |m| m.relation }
    end
    
    def get_base_type(index)
      with_metadata(index){ |m| m.type }
    end
  
    def column_count
      statement.metadata.size
    end
    
    def active?
      @active
    end
    
    def exhausted?
      !active?
    end
  private
    def with_metadata(index)
      if index >= 0 and index < column_count
        yield statement.metadata[index]
      end
    end
  end
end
