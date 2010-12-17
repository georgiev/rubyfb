module Arel
  module Visitors
    class RubyFB < Arel::Visitors::ToSql
    	Arel::Visitors::VISITORS['rubyfb'] = Arel::Visitors::RubyFB
    private
      def visit_Arel_Nodes_SelectStatement o
      	limit = o.limit; o.limit = nil;
      	offset = o.offset; o.offset = nil;
      	limit || offset ? "SELECT #{fb_limit(limit)} #{fb_offset(offset)} * FROM (#{super})" : super
      end

      def fb_limit limit
      	"first #{limit}" if limit
      end

      def fb_offset offset
      	"skip #{offset}" if offset
      end
    end
  end
end
