/** 
 * @file importtracker.h
 * @brief A utility for importing linksets from XML.
 * Discrete wuz here
 */
#ifndef IMPORTTRACKER_H
#define IMPORTTRACKER_H
class ImportTracker
{
	public:
		enum ImportState { IDLE, WAND };
		ImportTracker()
		: numberExpected(0),
		state(IDLE)
		{ }
		ImportTracker(LLSD &data) { state = IDLE; numberExpected=0;}
		~ImportTracker() { }
		void expectRez();
		void get_update(S32 newid, BOOL justCreated = false, BOOL createSelected = false);
		const int getState() { return state; }
	private:
		int				numberExpected;
		int				state;
};
extern ImportTracker gImportTracker;
#endif
