#pragma once

namespace Assets
{
	class IMaterial
	{
	public:
		virtual ~IMaterial() = default;
		class IInstance
		{
		public:
			virtual ~IInstance() = default;
		};
	};
}