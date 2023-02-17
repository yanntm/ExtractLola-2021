foreach i (INPUTS/*/)
	echo LAUNCHING LTL FIREABILITY $i
	date +"%T"
	rm -f ${i}LTLFireability.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}LTLFireability.xml >& ${i}LTLFireability.log || true
	pkill -9 -f "sara.*LTLFireability" 
end
echo LTL FIREABLITY FINISHED
