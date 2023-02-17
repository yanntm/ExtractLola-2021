foreach i (INPUTS/*/)
	echo LAUNCHING ONESAFE $i
	date +"%T"
	rm -f ${i}OneSafe.log
	./lola --conf=myconf ${i}model.pnml --check=OneSafe >& ${i}OneSafe.log || true
end
echo GLOBAL PROPERTIES FINISHED
