class PipeStage
{
public:
	virtual void emit(void *data) = 0;
	virtual void finalize() = 0;
	inline PipeStage *getNextStage()
	{
		return mNext;
	}

private:
	DrawEngine *mDE;
	PipeStage *mNext;
	string mName;
};
