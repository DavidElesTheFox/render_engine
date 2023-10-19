#pragma once

namespace Assets
{
	class IMaterial
	{
	public:

		class IInstance
		{
		public:
			virtual ~IInstance() = default;

		};

		virtual ~IMaterial() = default;
		virtual const std::string& getName() const = 0;

	};
}