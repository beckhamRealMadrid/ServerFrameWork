#pragma once

#include "SectionObject.h"

START_NAMESPACE

/* Synchorinize Class */

class RENET_API Synchronized	
{
public:
	Synchronized( SectionObject *so );
	~Synchronized();

private:
	SectionObject* sectionObject;
};

inline Synchronized::Synchronized( SectionObject *so )
{
	sectionObject = so;

	sectionObject->EnterSection();
}

inline Synchronized::~Synchronized()
{
	sectionObject->LeaveSection();
}

END_NAMESPACE