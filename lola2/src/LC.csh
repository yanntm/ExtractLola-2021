foreach i (INPUTS/*/)
	echo LAUNCHING LTL CARDINALITY $i
	date +"%T"
	rm -f ${i}LTLCardinality.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}LTLCardinality.xml >& ${i}LTLCardinality.log || true
	pkill -9 -f "sara.*LTLCardinality" 
end
echo LTL CARDINALITY FINISHED
