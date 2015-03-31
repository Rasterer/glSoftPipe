class PipeStage
{
public:
	PipeStage(string &name);
	virtual void emit(void *data) = 0;
	virtual void finalize() = 0;

	inline void setNextStage(PipeStage *stage)
	{
		mNext = stage;
	}

	inline PipeStage *getNextStage()
	{
		return mNext;
	}

private:
	DrawEngine *mDE;
	PipeStage *mNext;
	string mName;
};
