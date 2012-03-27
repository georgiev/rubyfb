module Rubyfb
  class Statement
    class ColumnMetadata
      attr_reader :name, :alias, :key, :type, :scale, :relation
    end
    
    def create_column_metadata
      ColumnMetadata.new
    end
  end
end
