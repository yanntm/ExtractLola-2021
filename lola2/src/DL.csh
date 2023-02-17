foreach i (INPUTS/*/)
	echo LAUNCHING DEADLOCK $i
	date +"%T"
	rm -f ${i}Deadlock.log
	./lola --conf=myconf ${i}model.pnml --formula=${i}GlobalProperties.xml >& ${i}Deadlock.log || true
end
echo GLOBAL PROPERTIES FINISHED
