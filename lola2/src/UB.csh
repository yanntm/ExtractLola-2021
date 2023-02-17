foreach i (INPUTS/*/)
	echo LAUNCHING UPPER BOUNDS $i
	date +"%T"
	rm -f ${i}UpperBounds.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}UpperBounds.xml >& ${i}UpperBounds.log
	pkill -9 -f "sara.*UpperBounds" 
end
echo UPPER BOUNDS FINISHED
