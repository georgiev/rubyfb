module Rubyfb
  class Statement
    attr_reader :metadata
    
    class ColumnMetadata
      attr_reader :name, :alias, :key, :type, :scale, :relation
    end
    
    def create_column_metadata
      ColumnMetadata.new
    end
    
    def create_result_set(transaction)
      ResultSet.new(self, transaction)
    end
    
    def is_active_result_set(object)
      object.kind_of?(ResultSet) && object.active?
    end
  end
end
