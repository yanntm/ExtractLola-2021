foreach i (INPUTS/*/)
	echo LAUNCHING $i
	date +"%T"
	rm -f ${i}Deadlock.log
	rm -f ${i}HomeStates.log
	rm -f ${i}Reversibility.log
	rm -f ${i}QuasiLifeness.log
	rm -f ${i}Lifeness.log
	rm -f ${i}OneSafe.log
	rm -f ${i}StableMarking.log
	echo DEADLOCK $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --formula=${i}GlobalProperties.xml >& ${i}Deadlock.log || true
	echo HOMESTATES $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --check=homestates >& ${i}HomeStates.log || true
	echo REVERSIBILITY $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --check=reversibility >& ${i}Reversibility.log || true
	echo QUASILIFENESS $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --check=QuasiLiveness >& ${i}QuasiLiveness.log || true
	echo LIFENESS $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --check=Liveness >& ${i}Liveness.log || true
	echo ONESAFE $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --check=OneSafe >& ${i}OneSafe.log || true
	echo STABLEMARKING $i
	date +"%T"
	./lola --conf=myconf ${i}model.pnml --check=StableMarking >& ${i}StableMarking.log || true
end
echo GLOBAL PROPERTIES FINISHED
