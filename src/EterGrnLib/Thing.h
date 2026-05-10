#pragma once

#include "Model.h"
#include "Motion.h"
#include <memory>

class CGraphicThing : public CResource
{
	public:
		typedef CRef<CGraphicThing> TRef;

	public:
		static CGraphicThing::TType Type();

public:
	bool LoadFromMemoryForEditor(int size, const void* data)
	{
		OnClear();
		return OnLoad(size, data);
	}
	public:
		CGraphicThing(const char * c_szFileName);
		virtual ~CGraphicThing();

		virtual bool			CreateDeviceObjects();
		virtual void			DestroyDeviceObjects();

		bool					CheckModelIndex(int iModel) const;
		CGrannyModel *			GetModelPointer(int iModel);
		int						GetModelCount() const;

		bool					CheckMotionIndex(int iMotion) const;
		CGrannyMotion *			GetMotionPointer(int iMotion);
		int						GetMotionCount() const;

	protected:
		void					Initialize();

		bool					LoadModels();
		bool					LoadMotions();

	protected:
		bool					OnLoad(int iSize, const void* c_pvBuf);
		void					OnClear();
		bool					OnIsEmpty() const;
		bool					OnIsType(TType type);

	protected:
		granny_file *			m_pgrnFile;
		granny_file_info *		m_pgrnFileInfo;

		granny_animation *		m_pgrnAni;

		std::unique_ptr<CGrannyModel[]>	m_models;
		std::unique_ptr<CGrannyMotion[]>	m_motions;
};
