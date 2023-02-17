foreach i (INPUTS/*/)
	echo LAUNCHING CTL FIREABILITY $i
	date +"%T"
	rm -f ${i}CTLFireability.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}CTLFireability.xml >& ${i}CTLFireability.log || true
	pkill -9 -f "sara.*CTLFireability" 
end
echo CTL FIREABLITY FINISHED
