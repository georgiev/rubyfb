module Arel
  module Visitors
    class RubyFB_15Compat < Arel::Visitors::ToSql
    	Arel::Visitors::VISITORS['rubyfb'] = Arel::Visitors::RubyFB_15Compat
    private
      def visit_Arel_Nodes_SelectStatement o
      	limit = o.limit; o.limit = nil;
      	offset = o.offset; o.offset = nil;
      	super.tap do |s|
      	  if limit || offset  
      	    s.gsub!(/^\s*select/i, "SELECT #{fb_limit(limit)} #{fb_offset(offset)} ")
      	  end
      	end
      end

      def fb_limit limit
      	"first #{limit.expr}" if limit
      end

      def fb_offset offset
      	"skip #{offset.expr}" if offset
      end
    end
  end
end
