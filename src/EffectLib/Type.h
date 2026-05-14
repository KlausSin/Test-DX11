#pragma once

enum EEffectType
{
	EFFECT_TYPE_PARTICLE = 1,
	EFFECT_TYPE_ANIMATION_TEXTURE = 2,
	EFFECT_TYPE_MESH = 3,
	EFFECT_TYPE_SIMPLE_LIGHT = 4,
};

enum EMeshBillBoardType
{
	MESH_BILLBOARD_TYPE_NONE,
	MESH_BILLBOARD_TYPE_ALL,
	MESH_BILLBOARD_TYPE_Y,
	MESH_BILLBOARD_TYPE_MOVE
};

enum EBillBoardType
{
	BILLBOARD_TYPE_NONE,
	BILLBOARD_TYPE_ALL,
	BILLBOARD_TYPE_Y,
	BILLBOARD_TYPE_LIE,
	BILLBOARD_TYPE_2FACE,
	BILLBOARD_TYPE_3FACE,
};

enum EMovingType
{
	MOVING_TYPE_DIRECT,
	MOVING_TYPE_BEZIER_CURVE,
};

template <typename T>
struct CTimeEvent
{
	typedef T value_type;

	float m_fTime;
	T m_Value;
};

template <typename T>
bool operator<(const CTimeEvent<T>& lhs, const CTimeEvent<T>& rhs)
{
	return lhs.m_fTime < rhs.m_fTime;
}

template <typename T>
bool operator<(const CTimeEvent<T>& lhs, const float& rhs)
{
	return lhs.m_fTime < rhs;
}

template <typename T>
bool operator<(const float& lhs, const CTimeEvent<T>& rhs)
{
	return lhs < rhs.m_fTime;
}

typedef struct SEffectPosition : public CTimeEvent<XMFLOAT3>
{
	int m_iMovingType;
	XMFLOAT3 m_vecControlPoint;
} TEffectPosition;

typedef CTimeEvent<char> TTimeEventTypeCharacter;
typedef CTimeEvent<short> TTimeEventTypeShort;
typedef CTimeEvent<float> TTimeEventTypeFloat;
typedef CTimeEvent<WORD> TTimeEventTypeWord;
typedef CTimeEvent<DWORD> TTimeEventTypeDoubleWord;
typedef CTimeEvent<XMFLOAT4> TTimeEventTypeColor;
typedef CTimeEvent<XMFLOAT2> TTimeEventTypeVector2;
typedef CTimeEvent<XMFLOAT3> TTimeEventTypeVector3;

typedef std::vector<float> TTimeEventTable;
typedef std::vector<TEffectPosition> TTimeEventTablePosition;
typedef std::vector<TTimeEventTypeCharacter> TTimeEventTableCharacter;
typedef std::vector<TTimeEventTypeShort> TTimeEventTableShort;
typedef std::vector<TTimeEventTypeFloat> TTimeEventTableFloat;
typedef std::vector<TTimeEventTypeWord> TTimeEventTableWord;
typedef std::vector<TTimeEventTypeDoubleWord> TTimeEventTableDoubleWord;
typedef std::vector<TTimeEventTypeColor> TTimeEventTableColor;
typedef std::vector<TTimeEventTypeVector2> TTimeEventTableVector2;
typedef std::vector<TTimeEventTypeVector3> TTimeEventTableVector3;

template <typename T>
T BlendSingleValue(float time, const CTimeEvent<T>& low, const CTimeEvent<T>& high)
{
	const float timeDiff = high.m_fTime - low.m_fTime;
	const float perc = (time - low.m_fTime) / timeDiff;

	const T valueDiff = high.m_Value - low.m_Value;
	return static_cast<T>(low.m_Value + perc * valueDiff);
}

inline XMFLOAT2 BlendSingleValue(float time, const CTimeEvent<XMFLOAT2>& low, const CTimeEvent<XMFLOAT2>& high)
{
	const float timeDiff = high.m_fTime - low.m_fTime;
	const float perc = (time - low.m_fTime) / timeDiff;

	return {
		low.m_Value.x + (high.m_Value.x - low.m_Value.x) * perc,
		low.m_Value.y + (high.m_Value.y - low.m_Value.y) * perc
	};
}

inline XMFLOAT3 BlendSingleValue(float time, const CTimeEvent<XMFLOAT3>& low, const CTimeEvent<XMFLOAT3>& high)
{
	const float timeDiff = high.m_fTime - low.m_fTime;
	const float perc = (time - low.m_fTime) / timeDiff;

	return {
		low.m_Value.x + (high.m_Value.x - low.m_Value.x) * perc,
		low.m_Value.y + (high.m_Value.y - low.m_Value.y) * perc,
		low.m_Value.z + (high.m_Value.z - low.m_Value.z) * perc
	};
}

inline XMFLOAT4 BlendSingleValue(float time, const CTimeEvent<XMFLOAT4>& low, const CTimeEvent<XMFLOAT4>& high)
{
	const float timeDiff = high.m_fTime - low.m_fTime;
	const float perc = (time - low.m_fTime) / timeDiff;

	return {
		low.m_Value.x + (high.m_Value.x - low.m_Value.x) * perc,
		low.m_Value.y + (high.m_Value.y - low.m_Value.y) * perc,
		low.m_Value.z + (high.m_Value.z - low.m_Value.z) * perc,
		low.m_Value.w + (high.m_Value.w - low.m_Value.w) * perc
	};
}

inline XMFLOAT3 BlendSingleValue(float time, const TEffectPosition& low, const TEffectPosition& high)
{
	const float timeDiff = high.m_fTime - low.m_fTime;
	const float perc = (time - low.m_fTime) / timeDiff;

	if (low.m_iMovingType == MOVING_TYPE_DIRECT)
	{
		return {
			low.m_Value.x + (high.m_Value.x - low.m_Value.x) * perc,
			low.m_Value.y + (high.m_Value.y - low.m_Value.y) * perc,
			low.m_Value.z + (high.m_Value.z - low.m_Value.z) * perc
		};
	}

	if (low.m_iMovingType == MOVING_TYPE_BEZIER_CURVE)
	{
		const float invPerc = 1.0f - perc;

		return {
			low.m_Value.x * invPerc * invPerc + (low.m_Value.x + low.m_vecControlPoint.x) * invPerc * perc * 2.0f + high.m_Value.x * perc * perc,
			low.m_Value.y * invPerc * invPerc + (low.m_Value.y + low.m_vecControlPoint.y) * invPerc * perc * 2.0f + high.m_Value.y * perc * perc,
			low.m_Value.z * invPerc * invPerc + (low.m_Value.z + low.m_vecControlPoint.z) * invPerc * perc * 2.0f + high.m_Value.z * perc * perc
		};
	}

	return {};
}

template <typename T>
auto GetTimeEventBlendValue(float time, const std::vector<T>& vec) -> typename T::value_type
{
	if (vec.empty())
		return typename T::value_type();

	if (vec.begin() + 1 == vec.end())
		return vec.front().m_Value;

	if (time < vec.front().m_fTime)
		return vec.front().m_Value;

	if (time > vec.back().m_fTime)
		return vec.back().m_Value;

	auto result = std::equal_range(vec.begin(), vec.end(), time);

	if (result.first != result.second)
		return result.first->m_Value;

	--result.first;
	return BlendSingleValue(time, *result.first, *result.second);
}

extern BOOL GetTokenTimeEventFloat(CTextFileLoader& rTextFileLoader, const char* c_szKey, TTimeEventTableFloat* pTimeEventTableFloat);

template <typename T>
void InsertItemTimeEvent(std::vector<CTimeEvent<T>>* pTable, float fTime, T fValue)
{
	typedef typename std::vector<CTimeEvent<T>>::iterator iterator;

	iterator itor = std::lower_bound(pTable->begin(), pTable->end(), fTime);

	CTimeEvent<T> TimeEvent;
	TimeEvent.m_fTime = fTime;
	TimeEvent.m_Value = fValue;

	pTable->insert(itor, TimeEvent);
}
