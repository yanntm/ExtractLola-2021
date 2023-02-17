foreach i (INPUTS/*/)
	echo LAUNCHING CTL CARDINALITY $i
	date +"%T"
	rm -f ${i}CTLCardinality.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}CTLCardinality.xml >& ${i}CTLCardinality.log || true
	pkill -9 -f "sara.*CTLCardinality" 
end
echo CTL CARDINALITY FINISHED
