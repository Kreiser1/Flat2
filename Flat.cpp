#define GLEW_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define GLT_IMPLEMENTATION
#define GLT_MANUAL_VIEWPORT
#define _WINSOCKAPI_
#define _USE_MATH_DEFINES

#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <algorithm>
#include <list>
#include <filesystem>

#include "Lua/lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "LuaBridge/RefCountedPtr.h"
#include "GLEW/glew.h"
#include "GLFW/glfw3.h"
#include "STB/stb_image.h"
#include "GLText/gltext.h"
#include "HttpLib/httplib.h"

/*
BSD 3-Clause License

Copyright (c) 2025, Syrtsov Vadim

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Credits to: GLFW, Lua, GLEW, GLAD, GLText, LuaBridge, stb.
*/

#define FLAT_VERSION_STRING "2.5"

const volatile LPCSTR flatVersionString = "FLAT" FLAT_VERSION_STRING;
const volatile LPCSTR flatLicenseString = R"(
BSD 3-Clause License

Copyright (c) 2025, Syrtsov Vadim

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
)";

const volatile LPCSTR flatCreditsString = R"(Credits to: GLFW, Lua, GLEW, GLAD, GLText, LuaBridge, stb, httplib.)";

namespace Flat
{
	using namespace std;
	using namespace luabridge;
	using namespace httplib;

	struct Date
	{
		WORD wYear;
		BYTE bMonth, bDay, bHour, bMinute, bSecond;

		Date(WORD wYear, BYTE bMonth, BYTE bDay, BYTE bHour, BYTE bMinute, BYTE bSecond) : wYear(wYear), bMonth(bMonth), bDay(bDay), bHour(bHour), bMinute(bMinute), bSecond(bSecond) {}

		Date() : Date(0, 0, 0, 0, 0, 0) {}

		operator ULONGLONG()
		{
			return wYear * 12 * 30 * 24 * 60 * 60 + bMonth * 30 * 24 * 60 * 60 + bDay * 24 * 60 * 60 + bHour * 60 * 60 + bMinute * 60 + bSecond;
		}

		operator bool()
		{
			return operator ULONGLONG() && bMonth <= 12 && bDay <= 31 && bHour <= 24 && bMinute <= 60 && bSecond <= 60;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%02d-%02d-%04d %02d-%02d-%02d)", bDay, bMonth, wYear, bHour, bMinute, bSecond);
			return lpString;
		}

		bool operator==(Date other)
		{
			return operator ULONGLONG() == other.operator ULONGLONG();
		}

		bool operator<(Date other)
		{
			return operator ULONGLONG() < other.operator ULONGLONG();
		}

		bool operator>(Date other)
		{
			return !operator<(other);
		}

		bool operator<=(Date other)
		{
			return operator<(other) || operator==(other);
		}

		bool operator>=(Date other)
		{
			return operator>(other) || operator==(other);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Date>("date")
				.addConstructor<void (*)(WORD, BYTE, BYTE, BYTE, BYTE, BYTE)>()
				.addData("year", &Date::wYear)
				.addData("month", &Date::bMonth)
				.addData("day", &Date::bDay)
				.addData("hour", &Date::bHour)
				.addData("minute", &Date::bHour)
				.addData("second", &Date::bSecond)
				.addFunction("total", &operator ULONGLONG)
				.addFunction<bool, Date>("__eq", &operator==)
				.addFunction<bool, Date>("__lt", &operator<)
				.addFunction<bool, Date>("__le", &operator<=)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Clock
	{
	private:
		Clock() {}

	public:
		static Date localDate()
		{
			SYSTEMTIME localTime;
			GetLocalTime(&localTime);
			return Date(localTime.wYear, localTime.wMonth, localTime.wDay, localTime.wHour, localTime.wMinute, localTime.wSecond);
		}

		static Date systemDate()
		{
			SYSTEMTIME systemTime;
			GetSystemTime(&systemTime);
			return Date(systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond);
		}

		static void wait(float time)
		{
			Sleep(time * 1000.0f);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("clock")
				.addFunction<Date>("localDate", &localDate)
				.addFunction<Date>("systemDate", &systemDate)
				.addFunction<void, float>("wait", &wait)
				.endNamespace()
				.endNamespace();
		}
	};

	class Error
	{
	private:
		Error() {}

	public:
		static void raise(LPCSTR lpMessage)
		{
			printf("[ERROR %s]: %s\n", (LPCSTR)Clock::localDate(), lpMessage);

			if (MessageBox(nullptr, lpMessage, "Flat: Error.", MB_ICONERROR | MB_SYSTEMMODAL | MB_RETRYCANCEL) != IDRETRY)
				ExitProcess(1);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("error")
				.addFunction<void, LPCSTR>("raise", &raise)
				.endNamespace()
				.endNamespace();
		}
	};

	class Explorer
	{
	private:
		Explorer() {}

	public:
		static LPCSTR select(LPCSTR lpFilter)
		{
			OPENFILENAME openFileName;
			ZeroMemory(&openFileName, sizeof(OPENFILENAME));

			openFileName.lStructSize = sizeof(OPENFILENAME);
			openFileName.lpstrFilter = lpFilter;
			openFileName.lpstrFile = new CHAR[256];
			openFileName.lpstrFile[0] = '\0';
			openFileName.nMaxFile = 256;
			openFileName.lpstrTitle = "Flat: Select file.";
			openFileName.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_NODEREFERENCELINKS | OFN_NONETWORKBUTTON | OFN_NOLONGNAMES | OFN_DONTADDTORECENT;

			if (!GetOpenFileName(&openFileName))
				Error::raise("Failed to select file.");

			return openFileName.lpstrFile;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("explorer")
				.addFunction<LPCSTR, LPCSTR>("select", &select)
				.endNamespace()
				.endNamespace();
		}
	};

	class Stopwatch
	{
	private:
		float startTime;

	public:
		Stopwatch() : startTime(clock()) {}

		void reset()
		{
			startTime = clock();
		}

		float elapsed()
		{
			return (clock() - startTime) / CLOCKS_PER_SEC;
		}

		operator bool()
		{
			return elapsed();
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%f)", elapsed());
			return lpString;
		}

		bool operator==(Stopwatch other)
		{
			return elapsed() == other.elapsed();
		}

		bool operator<(Stopwatch other)
		{
			return elapsed() < other.elapsed();
		}

		bool operator>(Stopwatch other)
		{
			return !operator<(other);
		}

		bool operator<=(Stopwatch other)
		{
			return operator<(other) || operator==(other);
		}

		bool operator>=(Stopwatch& other)
		{
			return operator>(other) || operator==(other);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Stopwatch>("stopwatch")
				.addConstructor<void (*)()>()
				.addFunction<void>("reset", &reset)
				.addFunction<float>("elapsed", &elapsed)
				.addFunction<bool, Stopwatch>("__eq", &operator==)
				.addFunction<bool, Stopwatch>("__lt", &operator<)
				.addFunction<bool, Stopwatch>("__le", &operator<=)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Timer
	{
	private:
		float destinationTime;

	public:
		float delay;

		Timer(float delay) : delay(delay) { reset(); }

		Timer() : Timer(0.0f) {}

		void reset()
		{
			destinationTime = clock() + delay * CLOCKS_PER_SEC;
		}

		float left()
		{
			return !operator bool() ? (destinationTime - clock()) / CLOCKS_PER_SEC : 0.0f;
		}

		operator bool()
		{
			return clock() > destinationTime;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%f)", left());
			return lpString;
		}

		bool operator==(Timer other)
		{
			return left() == other.left();
		}

		bool operator<(Timer other)
		{
			return left() < other.left();
		}

		bool operator>(Timer other)
		{
			return !operator<(other);
		}

		bool operator<=(Timer other)
		{
			return operator<(other) || operator==(other);
		}

		bool operator>=(Timer other)
		{
			return operator>(other) || operator==(other);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Timer>("timer")
				.addConstructor<void (*)(float)>()
				.addData("delay", &Timer::delay)
				.addFunction<void>("reset", &reset)
				.addFunction<float>("left", &left)
				.addFunction<bool, Timer>("__eq", &operator==)
				.addFunction<bool, Timer>("__lt", &operator<)
				.addFunction<bool, Timer>("__le", &operator<=)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Math
	{
	private:
		Math() {}

	public:
		static const float epsilon, infinity;

		static float absolute(float value)
		{
			return fabsf(value);
		}

		static int round(float value)
		{
			return value - fmod(value, 1.0f);
		}

		static float squareRoot(float value)
		{
			if (value < 0)
				Error::raise("Negative square root.");

			return sqrtf(value);
		}

		static float power(float value, float power)
		{
			return powf(value, power);
		}

		static float average(float a, float b)
		{
			return (a + b) * 0.5f;
		}

		static float clamp(float value, float min, float max)
		{
			if (min == max)
				return min;

			if (min > max)
				Error::raise("Invalid clamp limits.");

			if (value < min)
				return min;

			if (value > max)
				return max;

			return value;
		}

		static float normalize(float value, float limit)
		{
			if (limit < 0)
				Error::raise("Invalid normalization limit.");

			while (value > limit)
				value -= limit * 2;

			while (value < -limit)
				value += limit * 2;

			return value;
		}

		static float random(float min, float max)
		{
			if (min > max)
				Error::raise("Invalid random limits.");

			return min + (float)rand() / (float)(RAND_MAX / (max - min));
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("math")
				.addConstant("epsilon", &epsilon)
				.addConstant("infinity", &infinity)
				.addFunction<float, float>("absolute", &absolute)
				.addFunction<int, float>("round", &round)
				.addFunction<float, float>("root", &squareRoot)
				.addFunction<float, float, float>("power", &power)
				.addFunction<float, float, float>("average", &average)
				.addFunction<float, float, float, float>("clamp", &clamp)
				.addFunction<float, float, float>("normalize", &normalize)
				.addFunction<float, float, float>("random", &random)
				.endNamespace()
				.endNamespace();
		}
	};

	const float Math::epsilon = std::numeric_limits<float>::epsilon();
	const float Math::infinity = std::numeric_limits<float>::infinity();

	class Geometry
	{
	private:
		Geometry() {}

	public:
		static const float pi;

		static float average(float x, float y)
		{
			return Math::squareRoot(x * y);
		}

		static float length(float x, float y)
		{
			return Math::squareRoot(x * x + y * y);
		}

		static float angle(float x, float y)
		{
			return atan2f(y, x) * 180 / pi;
		}

		static float sine(float value)
		{
			return sinf(value * (pi / 180));
		}

		static float cosine(float value)
		{
			return cosf(value * (pi / 180));
		}

		static float interpolate(float a, float b, float time)
		{
			return a * (1.0f - time) + b * time;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("geometry")
				.addConstant("pi", &pi)
				.addFunction<float, float, float>("average", &average)
				.addFunction<float, float, float>("length", &length)
				.addFunction<float, float, float>("angle", &angle)
				.addFunction<float, float>("sine", &sine)
				.addFunction<float, float>("cosine", &cosine)
				.addFunction<float, float, float, float>("interpolate", &interpolate)
				.endNamespace()
				.endNamespace();
		}
	};

	const float Geometry::pi = M_PI /*3.141592653589793238462643383279502884*/;

	class Vector
	{
	public:
		float x, y;

		Vector(float x, float y) : x(x), y(y) {}

		Vector(float d) : x(d), y(d) {}

		Vector() : Vector(0.0f, 0.0f) {}

		float length()
		{
			return Geometry::length(x, y);
		}

		float angle()
		{
			return Geometry::angle(x, y);
		}

		Vector unit()
		{
			return operator*(1.0f / length());
		}

		void rotate(float angle)
		{
			float x2 = x * Geometry::cosine(angle) - y * Geometry::sine(angle);
			y = x * Geometry::sine(angle) + y * Geometry::cosine(angle);
			x = x2;
		}

		void adjust(float length)
		{
			*this = unit() * length;
		}

		void interpolate(Vector other, float time)
		{
			x = Geometry::interpolate(x, other.x, time);
			y = Geometry::interpolate(y, other.y, time);
		}

		void clamp(Vector min, Vector max)
		{
			x = Math::clamp(x, min.x, max.x);
			y = Math::clamp(y, min.y, max.y);
		}

		void normalize(Vector limit)
		{
			x = Math::normalize(x, limit.x);
			y = Math::normalize(y, limit.y);
		}

		static Vector average(Vector a, Vector b)
		{
			return (a + b) * 0.5f;
		}

		operator bool()
		{
			return length();
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%f, %f)", x, y);
			return lpString;
		}

		bool operator==(Vector other)
		{
			return x == other.x && y == other.y;
		}

		bool operator<(Vector other)
		{
			return length() < other.length();
		}

		bool operator>(Vector other)
		{
			return !operator<(other);
		}

		bool operator<=(Vector other)
		{
			return operator<(other) || operator==(other);
		}

		bool operator>=(Vector other)
		{
			return operator>(other) || operator==(other);
		}

		Vector operator+(Vector other)
		{
			return Vector(x + other.x, y + other.y);
		}

		Vector operator-()
		{
			return Vector(-x, -y);
		}

		Vector operator-(Vector other)
		{
			return Vector(x - other.x, y - other.y);
		}

		Vector operator*(Vector other)
		{
			return Vector(x * other.x, y * other.y);
		}

		Vector operator*(float other)
		{
			return Vector(x * other, y * other);
		}

		void operator+=(Vector other)
		{
			*this = operator+(other);
		}

		void operator-=(Vector other)
		{
			operator+(-other);
		}

		void operator*=(Vector other)
		{
			*this = operator*(other);
		}

		void operator*=(float other)
		{
			*this = operator*(other);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Vector>("vector")
				.addConstructor<void (*)(float, float)>()
				.addData("x", &Vector::x)
				.addData("y", &Vector::y)
				.addFunction<float>("length", &length)
				.addFunction<float>("angle", &angle)
				.addFunction<Vector>("unit", &unit)
				.addFunction<void, float>("rotate", &rotate)
				.addFunction<void, float>("adjust", &adjust)
				.addFunction<void, Vector, float>("interpolate", &interpolate)
				.addFunction<void, Vector, Vector>("clamp", &clamp)
				.addFunction<void, Vector>("normalize", &normalize)
				.addStaticFunction<Vector, Vector, Vector>("average", function(&average))
				.addFunction<bool, Vector>("__eq", &operator==)
				.addFunction<bool, Vector>("__lt", &operator<)
				.addFunction<bool, Vector>("__le", &operator<=)
				.addFunction<Vector>("__unm", &operator-)
				.addFunction<Vector, Vector>("__add", &operator+)
				.addFunction<Vector, Vector>("__sub", &operator-)
				.addFunction<Vector, Vector>("__mul", &operator*)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Image
	{
	public:
		LPBYTE lpPixels;
		UINT nWidth, nHeight;
		GLuint glId;

		Image(LPCSTR lpFilePath)
		{
			lpPixels = stbi_load(lpFilePath, (int*)&nWidth, (int*)&nHeight, nullptr, 4);
			glId = GL_NONE;

			if (!lpPixels)
				Error::raise(stbi_failure_reason());
		}

		Image() : lpPixels(nullptr), nWidth(0), nHeight(0), glId(GL_NONE) {}

		float diagonal()
		{
			return Geometry::length(nWidth, nHeight);
		}

		void destroy()
		{
			STBI_FREE(lpPixels);
			lpPixels = nullptr;
			nWidth = 0;
			nHeight = 0;
			glId = GL_NONE;
		}

		operator bool()
		{
			return lpPixels && nWidth && nHeight;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%p, %u, %u, %u)", lpPixels, nWidth, nHeight, glId);
			return lpString;
		}

		bool operator==(Image other)
		{
			return lpPixels == other.lpPixels;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Image>("image")
				.addConstructor<void (*)(LPCSTR)>()
				.addData("width", &Image::nWidth, false)
				.addData("height", &Image::nHeight, false)
				.addFunction<void>("destroy", &destroy)
				.addFunction<bool, Image>("__eq", &operator==)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Transform
	{
	public:
		Vector position;
		Vector scale;
		float rotation;

		Transform(Vector position, Vector scale, float rotation) : position(position), scale(scale), rotation(rotation) {}

		Transform() : Transform(Vector(), Vector(), 0.0f) {}

		float diagonal()
		{
			return scale.length();
		}

		Vector center()
		{
			return position + scale * 0.5f;
		}

		static float distance(Transform a, Transform b)
		{
			return (b.center() - a.center()).length();
		}

		static float angle(Transform a, Transform b)
		{
			return (a.center() - b.center()).angle();
		}

		/*static bool intersect(Transform a, Transform b)
		{
			Vector verticesA[] =
			{
				a.position.x, a.position.y + a.scale.y,
				a.position + a.scale,
				a.position.x + a.scale.x, a.position.y,
				a.position
			};

			Vector verticesB[] =
			{
				b.position.x, b.position.y + b.scale.y,
				b.position + b.scale,
				b.position.x + b.scale.x, b.position.y,
				b.position
			};

			for (BYTE i = 0; i < 2; i++)
			{
				Vector* vertices = (!i ? verticesA : verticesB);

				for (BYTE j = 0; j < 4; j++)
				{
					INT l = (j + 1) % 4;
					Vector p1 = vertices[j];
					Vector p2 = vertices[l];

					Vector normal(p2.y - p1.y, p2.x - p1.x);

					float minA = INFINITY;
					float maxA = -INFINITY;

					for (BYTE k = 0; k < 4; k++)
					{
						float projected = normal.x * verticesA[k].x + normal.y * verticesA[k].y;

						if (projected < minA)
							minA = projected;
						if (projected > maxA)
							maxA = projected;
					}

					float minB = INFINITY;
					float maxB = -INFINITY;

					for (BYTE k = 0; k < 4; k++)
					{
						float projected = normal.x * verticesB[k].x + normal.y * verticesB[k].y;

						if (projected < minB)
							minB = projected;
						if (projected > maxB)
							maxB = projected;
					}

					if (maxA < minB || maxB < minA)
						return false;
				}
			}

			return true;
		}*/

		static bool intersect(Transform a, Transform b)
		{
			if (a.position.x + a.scale.x > b.position.x && a.position.x < b.position.x + b.scale.x && a.position.y + a.scale.y > b.position.y && a.position.y < b.position.y + b.scale.y)
				return true;

			return false;
		}

		void rotate(float angle)
		{
			scale.rotate(angle);
		}

		void adjust(float diagonal)
		{
			scale.adjust(diagonal);
		}

		void interpolate(Transform other, float time)
		{
			position.interpolate(other.position, time);
			scale.interpolate(other.scale, time);
			rotation = Geometry::interpolate(rotation, other.rotation, time);
		}

		void clamp(Vector min, Vector max)
		{
			scale.clamp(min, max);
		}

		operator bool()
		{
			return diagonal();
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%s, %s, %f)", position.operator LPCSTR(), scale.operator LPCSTR(), rotation);
			return lpString;
		}

		bool operator==(Transform other)
		{
			return position == other.position && scale == other.scale && rotation == other.rotation;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Transform>("transform")
				.addConstructor<void (*)(Vector, Vector, float)>()
				.addData("position", &Transform::position)
				.addData("scale", &Transform::scale)
				.addData("rotation", &Transform::rotation)
				.addFunction<float>("diagonal", &diagonal)
				.addFunction<Vector>("center", &center)
				.addFunction<void, float>("rotate", &rotate)
				.addFunction<void, float>("adjust", &adjust)
				.addFunction<void, Transform, float>("interpolate", &interpolate)
				.addFunction<void, Vector, Vector>("clamp", &clamp)
				.addStaticFunction<float, Transform, Transform>("distance", function(&distance))
				.addStaticFunction<float, Transform, Transform>("angle", function(&angle))
				.addStaticFunction<bool, Transform, Transform>("intersect", function(&intersect))
				.addFunction<bool, Transform>("__eq", &operator==)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Tile
	{
	public:
		Transform transform;
		Image texture;
		bool dynamic, tangible, pushable;
		float bounciness, friction;
		Vector velocity;

		Tile(Transform transform, Image texture, bool dynamic, bool tangible, bool pushable, float bounciness, float friction, Vector velocity) : texture(texture), transform(transform), dynamic(dynamic), tangible(tangible), pushable(pushable), bounciness(bounciness), friction(friction), velocity(velocity) {}

		Tile() : Tile(Transform(), Image(), false, false, false, 0.0f, 0.0f, Vector()) {}

		static bool phase(Tile tile1, Tile tile2)
		{
			if (tile1.transform == tile2.transform || !tile1.tangible || !tile2.tangible || !tile1.dynamic && !tile2.dynamic)
				return true;

			return false;
		}

		operator bool()
		{
			return texture && transform;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%s, %s, %s, %s, %s, %f, %f, %s)", transform.operator LPCSTR(), texture.operator LPCSTR(), dynamic ? "dynamic" : "static", tangible ? "tangible" : "intangible", pushable ? "pushable" : "impushable", bounciness, friction, velocity.operator LPCSTR());
			return lpString;
		}

		bool operator==(Tile other) const
		{
			return transform == other.transform && texture == other.texture && dynamic == other.dynamic && tangible == other.tangible && pushable == other.pushable && bounciness == other.bounciness && friction == other.friction && velocity == other.velocity;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Tile>("tile")
				.addConstructor<void (*)(Transform, Image, bool, bool, bool, float, float, Vector)>()
				.addData("transform", &Tile::transform)
				.addData("texture", &Tile::texture)
				.addData("dynamic", &Tile::dynamic)
				.addData("tangible", &Tile::tangible)
				.addData("pushable", &Tile::pushable)
				.addData("bounciness", &Tile::bounciness)
				.addData("friction", &Tile::friction)
				.addData("velocity", &Tile::velocity)
				.addStaticFunction<bool, Tile, Tile>("phase", function(&phase))
				.addFunction<bool, Tile>("__eq", &operator==)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	class Label
	{
	public:
		Vector position;
		float scale;
		LPCSTR lpText;
		ULONG uColor;
		GLTtext* gltText;

		Label(Vector position, float scale, LPCSTR lpText, ULONG uColor) : position(position), scale(scale), lpText(lpText), uColor(uColor), gltText(nullptr) {}

		Label() : Label(Vector(), 0.0f, nullptr, 0x00000000) {}

		operator bool()
		{
			return scale && lpText;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%s, %f, %s, 0x%X)", position.operator LPCSTR(), scale, lpText, uColor);
			return lpString;
		}

		bool operator==(Label other)
		{
			return position == other.position && scale == other.scale && lpText == other.lpText && uColor == other.uColor && gltText == other.gltText;
		}

		void reset()
		{
			gltDeleteText(gltText);
			gltText = nullptr;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginClass<Label>("label")
				.addConstructor<void (*)(Vector, float, LPCSTR, ULONG)>()
				.addData("position", &Label::position)
				.addData("scale", &Label::scale)
				.addData("text", &Label::lpText)
				.addData("color", &Label::uColor)
				.addFunction<void>("reset", &reset)
				.addFunction<bool, Label>("__eq", &operator==)
				.addFunction<LPCSTR>("__tostring", &operator LPCSTR)
				.endClass()
				.endNamespace();
		}
	};

	enum class EventType : UINT
	{
		Update = 1,
		Render,
		Phase,
		Collision,
		Keyboard,
		Mouse,
		Network,
		Invalid
	};

	class Event
	{
	public:
		EventType type;
		LPVOID lpParameters;

		Event(EventType type, LPVOID lpParameters) : type(type), lpParameters(lpParameters) {}
		Event() : type(EventType::Invalid), lpParameters(nullptr) {}

		operator bool()
		{
			return type < EventType::Invalid;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%u, %p)", type, lpParameters);
			return lpString;
		}

		bool operator==(Event other) const
		{
			return type == other.type && lpParameters == other.lpParameters;
		}

		void destroy()
		{
			if (lpParameters)
			{
				delete[] lpParameters;
				lpParameters = nullptr;
			}

			type = EventType::Invalid;
		}
	};

	class Hook
	{
	public:
		EventType type;
		LuaRef function;

		Hook(EventType type, LuaRef function) : type(type), function(function) {}
		Hook() : type(EventType::Invalid), function(nullptr) {}

		operator bool()
		{
			return type < EventType::Invalid;
		}

		operator LPCSTR()
		{
			LPSTR lpString = new CHAR[256];
			sprintf_s(lpString, 256, "(%u, %p)", type, function.tostring());
			return lpString;
		}

		bool operator==(Hook other) const
		{
			return type == other.type && function.operator==(other.function);
		}
	};

	class Lua
	{
	private:
		lua_State* L;

	public:
		Lua()
		{
			L = luaL_newstate();
			luaL_openlibs(L);

			lua_gc(L, LUA_GCSTOP);
			luaL_dostring(L, "collectgarbage = nil");

			getGlobalNamespace(L).beginNamespace("flat").addConstant("version", FLAT_VERSION_STRING).endNamespace();
		}

		template <typename... Args>
		void call(LuaRef& function, Args &&...args)
		{
			try
			{
				function(args...);
			}
			catch (exception e)
			{
				Error::raise(e.what());
			}
		}

		void loadFile(LPCSTR lpFilePath)
		{
			if (luaL_dofile(L, lpFilePath) != LUA_OK)
				Error::raise(lua_tostring(L, -1));
		}

		void loadModule(void (*lpModule)(Namespace))
		{
			try
			{
				lpModule(getGlobalNamespace(L).beginNamespace("flat"));
			}
			catch (exception e)
			{
				Error::raise(e.what());
			}
		}

		void collect()
		{
			lua_gc(L, LUA_GCCOLLECT);
		}

		void destroy()
		{
			lua_close(L);
			L = nullptr;
		}

		operator bool()
		{
			return L;
		}

		bool operator==(Lua other)
		{
			return L == other.L;
		}
	};

	class Console
	{
	private:
		static bool bRunning;

		Console() {}

	public:
		static bool running()
		{
			return bRunning;
		}

		static void destroy()
		{
			if (!bRunning || !ShowWindow(GetConsoleWindow(), SW_HIDE) || !FreeConsole())
				Error::raise("Failed to destroy console.");

			bRunning = false;
		}

		static void allocate()
		{
			if (bRunning)
				Error::raise("Failed to allocate console.");

			if (!AllocConsole())
				Error::raise("Failed to allocate console.");

			freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
			freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
			freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

			bRunning = true;
		}

		static void log(LPCSTR lpMessage)
		{
			printf("[LOG %s]: %s\n", (LPCSTR)Clock::localDate(), lpMessage);
		}

		static void print(LPCSTR lpMessage)
		{
			printf("%s\n", lpMessage);
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("console")
				.addVariable("running", &bRunning, false)
				.addFunction<void>("allocate", &Console::allocate)
				.addFunction<void>("destroy", &Console::destroy)
				.addFunction<void, LPCSTR>("log", &Console::log)
				.addFunction<void, LPCSTR>("print", &Console::print)
				.endNamespace()
				.endNamespace();
		}
	};

	bool Console::bRunning = false;

	class Dispatcher
	{
	private:
		Dispatcher() {}

		static list<Event> events;
		static list<Hook> hooks;

	public:
		static void hook(UINT type, LuaRef function)
		{
			Hook hook((EventType)type, function);

			if (!hook)
				Error::raise("Invalid hook.");

			hooks.push_back(hook);
		}

		static void unhook(UINT type, LuaRef function)
		{
			for_each(hooks.begin(), hooks.end(), [&](Hook& hook)
				{ if (hook.type == (EventType)type && hook.function.operator==(function)) hook.type = EventType::Invalid; });
		}

		static void sendEvent(EventType type, LPVOID lpParameters)
		{
			Event event(type, lpParameters);

			if (!event)
				Error::raise("Invalid event.");

			events.push_back(event);
		}

		static void pollEvents(Lua& lua, EventType type)
		{
			if (type >= EventType::Invalid)
			{
				hooks.remove_if([&](Hook& hook)
					{ return !hook; });
				events.remove_if([&](Event& event)
					{ return !event; });
				return;
			}

			try
			{
				for (Event& event : events)
					if (event.type == type)
					{
						for (Hook& hook : hooks)
							if (hook.type == type)
								switch (type)
								{
								case EventType::Update:
									lua.call(hook.function);
									break;
								case EventType::Render:
									lua.call(hook.function);
									break;
								case EventType::Phase:
									lua.call(hook.function, *((RefCountedPtr<Tile> **)event.lpParameters)[0], *((RefCountedPtr<Tile> **)event.lpParameters)[1]);
									break;
								case EventType::Collision:
									lua.call(hook.function, *((RefCountedPtr<Tile> **)event.lpParameters)[0], *((RefCountedPtr<Tile> **)event.lpParameters)[1], *((Vector**)event.lpParameters)[2]);
									break;
								case EventType::Keyboard:
									lua.call(hook.function, ((UINT*)event.lpParameters)[0], ((UINT*)event.lpParameters)[1]);
									break;
								case EventType::Mouse:
									lua.call(hook.function, ((UINT*)event.lpParameters)[0], ((UINT*)event.lpParameters)[1]);
									break;
								case EventType::Network:
									lua.call(hook.function, ((UINT*)event.lpParameters)[0]);
									break;
								}

						event.destroy();
					}
			}
			catch (exception e)
			{
				Error::raise(e.what());
			}
		}

		static void reset()
		{
			for_each(hooks.begin(), hooks.end(), [&](Hook& hook)
				{ hook.type = EventType::Invalid; });
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("engine")
				.beginNamespace("dispatcher")
				.addFunction<void, UINT, LuaRef>("hook", &hook)
				.addFunction<void, UINT, LuaRef>("unhook", &unhook)
				.addFunction<void>("reset", &reset)
				.addConstant("update", (UINT)EventType::Update)
				.addConstant("render", (UINT)EventType::Render)
				.addConstant("phase", (UINT)EventType::Phase)
				.addConstant("collision", (UINT)EventType::Collision)
				.addConstant("keyboard", (UINT)EventType::Keyboard)
				.addConstant("mouse", (UINT)EventType::Mouse)
				.addConstant("network", (UINT)EventType::Network)
				.endNamespace()
				.endNamespace()
				.endNamespace();
		}
	};

	list<Event> Dispatcher::events = list<Event>();
	list<Hook> Dispatcher::hooks = list<Hook>();

	enum class Protocol : UINT
	{
		Read,
		Write
	};

	class Network
	{
	private:
		static Server server;
		static bool bRunning;
		static USHORT port, clientPort;
		static HANDLE hListener;
		static string request, response, clientIP;
		static bool bServed;

		static void listener()
		{
			server.Get("/", [](const Request& req, Response& res)
				{
					while (!bServed);
					bServed = false;
					clientIP = req.remote_addr;
					clientPort = req.remote_port;
					request = string();
					Dispatcher::sendEvent(EventType::Network, new INT[]{ (INT)Protocol::Read });
					while (!bServed);
					res.set_content(response, "text/plain"); });

			server.Post("/", [](const Request& req, Response& res)
				{
					while (!bServed);
					bServed = false;
					clientIP = req.remote_addr;
					clientPort = req.remote_port;
					request = req.body;
					Dispatcher::sendEvent(EventType::Network, new INT[]{ (INT)Protocol::Write });
					while (!bServed);
					res.set_content(response, "text/plain"); });

			server.set_read_timeout(1.0f);
			server.set_write_timeout(1.0f);
			server.set_keep_alive_timeout(1.0f);

			port = server.bind_to_any_port("127.0.0.1", 0);

			bRunning = true;
			server.listen_after_bind();

			bRunning = false;
		}

		Network() {}

	public:
		static USHORT start()
		{
			if (bRunning)
				Error::raise("Failed to start network.");
			hListener = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)listener, nullptr, 0, nullptr);
			while (!bRunning)
				;
			bServed = true;
			return port;
		}

		static void serve(LPCSTR res)
		{
			if (!bRunning)
				Error::raise("Failed to serve.");

			response = res;
			bServed = true;
		}

		static LPCSTR read(LPCSTR ip, USHORT port)
		{
			Result result = Client(ip, port).Get("/");
			if (result && result->status == StatusCode::OK_200)
				return result->body.c_str();
			else
				return string().c_str();
		}

		static LPCSTR write(LPCSTR ip, USHORT port, LPCSTR data)
		{
			Result result = Client(ip, port).Post("/", data, "text/plain");
			if (result && result->status == StatusCode::OK_200)
				return result->body.c_str();
			else
				return string().c_str();
		}

		static bool running()
		{
			return bRunning;
		}

		static void destroy()
		{
			if (!bRunning)
				Error::raise("Failed to destroy network");

			server.stop();
			while (bRunning)
				;
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("network")
				.addVariable("port", &port, false)
				.addVariable("running", &bRunning, false)
				.addVariable("request", &request, false)
				.addVariable("response", &response, false)
				.beginNamespace("client")
				.addVariable("ip", &clientIP, false)
				.addVariable("port", &clientPort, false)
				.endNamespace()
				.addFunction<USHORT>("start", &Network::start)
				.addFunction<void>("destroy", &Network::destroy)
				.addFunction<void, LPCSTR>("serve", &Network::serve)
				.addFunction<LPCSTR, LPCSTR, USHORT>("read", &Network::read)
				.addFunction<LPCSTR, LPCSTR, USHORT, LPCSTR>("write", &Network::write)
				.beginNamespace("protocol")
				.addConstant("read", (UINT)Protocol::Read)
				.addConstant("write", (UINT)Protocol::Write)
				.endNamespace()
				.endNamespace()
				.endNamespace();
		}
	};

	bool Network::bRunning = false;
	Server Network::server = Server();
	USHORT Network::port = 0;
	USHORT Network::clientPort = 0;
	HANDLE Network::hListener = nullptr;
	string Network::request = string();
	string Network::response = string();
	string Network::clientIP = string();
	bool Network::bServed = false;

	void exit()
	{
		ExitProcess(0);
	}

	class Engine
	{
	private:
		Engine() {}

		static HANDLE hMain;
		static bool bRunning;
		static list<RefCountedPtr<Tile>> tiles;
		static list<RefCountedPtr<Label>> labels;
		static float fps, time, deltaTime, renderTime;
		static Vector gravity;
		static Lua lua;
		static GLFWwindow* glWindow;
		static bool lpKeys[GLFW_KEY_LAST + 1], lpButtons[GLFW_MOUSE_BUTTON_LAST + 2];
		static Vector cursorPosition;
		static ULONGLONG nFrames;
		static Transform camera;

		static void errorCallback(INT nCode, LPCSTR lpDescription)
		{
			Error::raise(lpDescription);
		}

		static void keyboardKeyCallback(GLFWwindow* glWindow, INT nKey, INT nScancode, INT nAction, INT nMods)
		{
			if (nKey == GLFW_KEY_UNKNOWN)
				return;

			if (nAction == GLFW_PRESS)
				lpKeys[nKey] = true;
			else if (nAction == GLFW_RELEASE)
				lpKeys[nKey] = false;

			Dispatcher::sendEvent(EventType::Keyboard, new INT[]{ nKey, nAction });
		}

		static void mouseButtonCallback(GLFWwindow* glWindow, INT nButton, INT nAction, INT nMods)
		{
			if (nAction == GLFW_PRESS)
				lpButtons[nButton] = true;
			else if (nAction == GLFW_RELEASE)
				lpButtons[nButton] = false;

			Dispatcher::sendEvent(EventType::Mouse, new INT[]{ nButton, nAction });
		}

		static void mouseCursorCallback(GLFWwindow* glWindow, double x, double y)
		{
			int nWidth, nHeight;
			glfwGetFramebufferSize(glWindow, &nWidth, &nHeight);

			cursorPosition = Vector((2.0f * x + 1.0f) / nWidth - 1.0f, (2.0f * (nHeight - y) + 1.0f) / nHeight - 1.0f) * camera.scale * 0.5f + camera.position + camera.scale * 0.5f;

			Dispatcher::sendEvent(EventType::Mouse, new INT[]{ GLFW_MOUSE_BUTTON_LAST, GLFW_RELEASE });
		}

		static void mouseScrollCallback(GLFWwindow* glWindow, double x, double y)
		{
			Dispatcher::sendEvent(EventType::Mouse, new INT[]{ GLFW_MOUSE_BUTTON_LAST + 1, y > 0 ? GLFW_PRESS : GLFW_RELEASE });
		}

		static void main(LPCSTR lpGameScript)
		{
			time = 0.0f;
			deltaTime = 0.0f;
			renderTime = 0.0f;

			tiles.clear();
			labels.clear();
			Dispatcher::reset();

			srand(::time(nullptr));

			if (!glfwInit())
				Error::raise("Failed to initialize OpenGL.");

			glfwSetErrorCallback(errorCallback);

			GLFWmonitor* glMonitor = glfwGetPrimaryMonitor();
			GLFWvidmode* glVideoMode = (GLFWvidmode*)glfwGetVideoMode(glMonitor);

			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

			glfwWindowHint(GLFW_RED_BITS, glVideoMode->redBits);
			glfwWindowHint(GLFW_GREEN_BITS, glVideoMode->greenBits);
			glfwWindowHint(GLFW_BLUE_BITS, glVideoMode->blueBits);

			glfwWindowHint(GLFW_ALPHA_BITS, 8);
			glfwWindowHint(GLFW_DEPTH_BITS, 24);
			glfwWindowHint(GLFW_STENCIL_BITS, 8);

			glfwWindowHint(GLFW_FLOATING, GL_TRUE);

			glWindow = glfwCreateWindow(glVideoMode->width * 0.75, glVideoMode->height * 0.75, "Flat " FLAT_VERSION_STRING, nullptr, nullptr);

			if (!glWindow)
				Error::raise("Failed to create window.");

			glfwMakeContextCurrent(glWindow);
			glfwShowWindow(glWindow);

			glfwSetWindowAttrib(glWindow, GLFW_FLOATING, GL_FALSE);

			if (glewInit())
				Error::raise("Failed to initialize GLEW.");

			if (!gltInit())
				Error::raise("Failed to initialize GLText.");

			nFrames = 0;
			fps = glVideoMode->refreshRate;
			camera = Transform(Vector(-1.0f, -1.0f), Vector(2.0f, 2.0f), 0.0f);

			Stopwatch updateStopwatch, renderStopwatch;
			Timer collectTimer(1.0f), renderTimer(1.0f / fps);

			glfwSetKeyCallback(glWindow, keyboardKeyCallback);
			glfwSetMouseButtonCallback(glWindow, mouseButtonCallback);
			glfwSetCursorPosCallback(glWindow, mouseCursorCallback);
			glfwSetScrollCallback(glWindow, mouseScrollCallback);

			glEnable(GL_TEXTURE_2D);
			glEnable(GL_BLEND);

			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			lua = Lua();

			lua.loadModule(&Date::luaModule);
			lua.loadModule(&Clock::luaModule);
			lua.loadModule(&Console::luaModule);
			lua.loadModule(&Error::luaModule);
			lua.loadModule(&Explorer::luaModule);
			lua.loadModule(&Stopwatch::luaModule);
			lua.loadModule(&Timer::luaModule);
			lua.loadModule(&Math::luaModule);
			lua.loadModule(&Geometry::luaModule);
			lua.loadModule(&Vector::luaModule);
			lua.loadModule(&Transform::luaModule);
			lua.loadModule(&Image::luaModule);
			lua.loadModule(&Tile::luaModule);
			lua.loadModule(&Label::luaModule);
			lua.loadModule(&Dispatcher::luaModule);
			lua.loadModule(&Network::luaModule);
			lua.loadModule(&Engine::luaModule);

			bRunning = true;

			lua.loadFile(lpGameScript);

			updateStopwatch.reset();
			renderStopwatch.reset();
			renderTimer.reset();
			collectTimer.reset();

			while (!glfwWindowShouldClose(glWindow))
			{
				deltaTime = Math::clamp(updateStopwatch.elapsed(), 0.0f, 1.0f);
				time += deltaTime;

				updateStopwatch.reset();

				Dispatcher::pollEvents(lua, EventType::Invalid);

				Dispatcher::pollEvents(lua, EventType::Network);

				glfwPollEvents();

				Dispatcher::pollEvents(lua, EventType::Keyboard);
				Dispatcher::pollEvents(lua, EventType::Mouse);

				Dispatcher::sendEvent(EventType::Update, nullptr);
				Dispatcher::pollEvents(lua, EventType::Update);

				for (RefCountedPtr<Tile>& tile : tiles)
					if (**tile)
					{
						if (!tile->dynamic)
							continue;

						tile->velocity += gravity * deltaTime;

						Vector movement = tile->velocity * deltaTime;

						tile->transform.position.x += movement.x;

						for (RefCountedPtr<Tile>& tile2 : tiles)
							if (**tile2)
								if (tile->transform != tile2->transform && Transform::intersect(tile->transform, tile2->transform))
								{
									if (Tile::phase(**tile, **tile2))
										Dispatcher::sendEvent(EventType::Phase, new LPVOID[]{ &tile, &tile2 });
									else
									{
										Vector impulse = (tile->velocity + tile2->velocity) * Vector(0.5f, 0.0f);

										Dispatcher::sendEvent(EventType::Collision, new LPVOID[]{ &tile, &tile2, &impulse });

										tile->transform.position.x -= movement.x;

										if (tile2->pushable)
											tile2->velocity.x = impulse.x;

										tile->velocity.x = -tile->velocity.x * tile->bounciness;
										tile->velocity.y -= tile->velocity.y * tile->friction;
									}
								}

						tile->transform.position.y += movement.y;

						for (RefCountedPtr<Tile>& tile2 : tiles)
							if (**tile2)
								if (tile->transform != tile2->transform && Transform::intersect(tile->transform, tile2->transform))
								{
									if (Tile::phase(**tile, **tile2))
										Dispatcher::sendEvent(EventType::Phase, new LPVOID[]{ &tile, &tile2 });
									else
									{
										Vector impulse = (tile->velocity + tile2->velocity) * Vector(0.0f, 0.5f);

										Dispatcher::sendEvent(EventType::Collision, new LPVOID[]{ &tile, &tile2, &impulse });

										tile->transform.position.y -= movement.y;

										if (tile2->pushable)
											tile2->velocity.y = impulse.y;

										tile->velocity.y = -tile->velocity.y * tile->bounciness;
										tile->velocity.x -= tile->velocity.x * tile->friction;
									}
								}
					}

				Dispatcher::pollEvents(lua, EventType::Phase);
				Dispatcher::pollEvents(lua, EventType::Collision);

				renderTimer.delay = 1.0f / fps;

				if (renderTimer.left() == 0.0f)
				{
					renderTime = Math::clamp(renderStopwatch.elapsed(), 0.0f, 1.0f);
					renderStopwatch.reset();

					Dispatcher::sendEvent(EventType::Render, nullptr);
					Dispatcher::pollEvents(lua, EventType::Render);

					UINT nWidth, nHeight;
					glfwGetWindowSize(glWindow, (INT*)&nWidth, (INT*)&nHeight);

					glViewport(0, 0, nWidth, nHeight);
					gltViewport(nWidth, nHeight);

					glUseProgram(GL_NONE);

					glLoadIdentity();
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					Transform scaledCamera = camera;

					scaledCamera.scale *= 0.5f;
					scaledCamera.position += scaledCamera.scale;
					scaledCamera.position *= -1.0f;
					scaledCamera.rotation = Math::normalize(scaledCamera.rotation, 180.0f);

					glScalef(1.0f / scaledCamera.scale.x, 1.0f / scaledCamera.scale.y, 0.0f);
					glRotatef(Math::normalize(scaledCamera.rotation, 180), 0.0f, 0.0f, 1.0f);
					glTranslatef(scaledCamera.position.x, scaledCamera.position.y, 0.0f);

					for (RefCountedPtr<Tile>& tile : tiles)
						if (**tile)
						{
							if (!tile->texture.glId)
							{
								glGenTextures(1, &tile->texture.glId);
								glBindTexture(GL_TEXTURE_2D, tile->texture.glId);

								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
								glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

								glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tile->texture.nWidth, tile->texture.nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, tile->texture.lpPixels);
							}

							glPushMatrix();

							Transform scaledTransform = tile->transform;
							scaledTransform.scale *= 0.5f;
							scaledTransform.position += scaledTransform.scale;
							scaledTransform.rotation = Math::normalize(scaledTransform.rotation, 180.0f);

							glTranslatef(scaledTransform.position.x, scaledTransform.position.y, 0.0f);
							glRotatef(tile->transform.rotation, 0.0f, 0.0f, 1.0f);

							glBindTexture(GL_TEXTURE_2D, tile->texture.glId);

							glBegin(GL_QUADS);

							glVertex2f(-scaledTransform.scale.x, scaledTransform.scale.y);
							glTexCoord2f(0.0f, 1.0f);
							glVertex2f(-scaledTransform.scale.x, -scaledTransform.scale.y);
							glTexCoord2f(1.0f, 1.0f);
							glVertex2f(scaledTransform.scale.x, -scaledTransform.scale.y);
							glTexCoord2f(1.0f, 0.0f);
							glVertex2f(scaledTransform.scale.x, scaledTransform.scale.y);
							glTexCoord2f(0.0f, 0.0f);

							glEnd();

							glPopMatrix();
						}

					gltBeginDraw();

					for (RefCountedPtr<Label>& label : labels)
						if (**label)
						{
							if (!label->gltText)
							{
								label->gltText = gltCreateText();
								gltSetText(label->gltText, label->lpText);
							}

							Vector scaledPosition = Vector(1.0f + (label->position.x + scaledCamera.position.x) / scaledCamera.scale.x, 1.0f - (label->position.y + scaledCamera.position.y) / scaledCamera.scale.y) * Vector(nWidth, nHeight) * 0.5f;
							float scaledScale = label->scale * label->scale / scaledCamera.scale.length() / gltGetLineHeight(label->scale) * Geometry::length(nWidth, nHeight) * 0.5f;

							gltColor((float)(label->uColor >> 16 & 0xFF) / 255.0f, (float)(label->uColor >> 8 & 0xFF) / 255.0f, (float)(label->uColor & 0xFF) / 255.0f, 1.0f);
							gltDrawText2DAligned(label->gltText, scaledPosition.x, scaledPosition.y, scaledScale, GLT_LEFT, GLT_BOTTOM);
						}

					gltEndDraw();
					glfwSwapBuffers(glWindow);

					renderTimer.reset();
				}

				if (collectTimer.left() == 0.0)
				{
					// lua.collect();
					collectTimer.reset();
				}
			}

			lua.destroy();

			Dispatcher::reset();

			if (Network::running())
				Network::destroy();

			if (Console::running())
				Console::destroy();

			tiles.clear();
			labels.clear();

			glfwTerminate();
			gltTerminate();
			bRunning = false;
		}

	public:
		static void start(LPCSTR lpGameScript)
		{
			if (bRunning)
				Error::raise("Engine is already running.");

			hMain = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)lpGameScript, 0, nullptr);
			while (!bRunning);
		}

		static void addTile(RefCountedPtr<Tile> tile)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			tiles.push_back(tile);
		}

		static void removeTile(RefCountedPtr<Tile> tile)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			tiles.remove(tile);
		}

		static RefCountedPtr<Tile> getTile(ULONGLONG nIndex)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (!nIndex || nIndex > tiles.size())
				Error::raise("Invalid tile index.");

			auto front = tiles.begin();
			advance(front, nIndex - 1);

			return *front;
		}

		static void addLabel(RefCountedPtr<Label> label)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			labels.push_back(label);
		}

		static void removeLabel(RefCountedPtr<Label> label)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			labels.remove(label);
		}

		static RefCountedPtr<Label> getLabel(ULONGLONG nIndex)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (!nIndex || nIndex > tiles.size())
				Error::raise("Invalid label index.");

			auto front = labels.begin();
			advance(front, nIndex - 1);

			return *front;
		}

		static void resetTiles()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			tiles.clear();
		}

		static void resetLabels()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			labels.clear();
		}

		static void playSound(LPCSTR lpSound)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (!PlaySound(lpSound, nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT))
				Error::raise("Failed to play sound.");
		}

		static void loopSound(LPCSTR lpSound)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (!PlaySound(lpSound, nullptr, SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT))
				Error::raise("Failed to play sound.");
		}

		static void resetSound()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (!PlaySound(nullptr, nullptr, false))
				Error::raise("Failed to reset sound.");
		}

		static void title(LPCSTR lpTitle)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			glfwSetWindowTitle(glWindow, lpTitle);
		}

		static void icon(Image icon)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			GLFWimage image;
			image.pixels = icon.lpPixels;
			image.width = icon.nWidth;
			image.height = icon.nHeight;

			glfwSetWindowIcon(glWindow, 1, &image);
		}

		static void showCursor()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		static void hideCursor()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}

		static void view(Transform view)
		{
			camera = view;
		}

		static bool running()
		{
			return bRunning;
		}

		static void destroy()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			glfwSetWindowShouldClose(glWindow, GLFW_TRUE);
		}

		static bool key(USHORT nKey)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (nKey >= GLFW_KEY_LAST)
				Error::raise("Invalid key.");

			return lpKeys[nKey];
		}

		static bool button(BYTE nButton)
		{
			if (!bRunning)
				Error::raise("Engine is not running.");

			if (nButton >= GLFW_MOUSE_BUTTON_LAST)
				Error::raise("Invalid button.");

			return lpButtons[nButton];
		}

		static void join()
		{
			if (!bRunning)
				Error::raise("Engine is not running.");
			WaitForSingleObject(hMain, INFINITE);
		}

		static int tileCount()
		{
			return tiles.size();
		}

		static int labelCount()
		{
			return labels.size();
		}

		static void luaModule(Namespace flat)
		{
			flat.beginNamespace("engine")
				.addVariable("time", &time, false)
				.addVariable("fps", &fps)
				.addVariable("deltaTime", &deltaTime, false)
				.addVariable("renderTime", &renderTime, false)
				.addVariable("gravity", &gravity)
				.beginNamespace("tile")
				.addFunction<void, RefCountedPtr<Tile>>("add", &addTile)
				.addFunction<void, RefCountedPtr<Tile>>("remove", &removeTile)
				.addFunction<RefCountedPtr<Tile>, ULONGLONG>("get", &getTile)
				.addFunction<void>("reset", &resetTiles)
				.addFunction<int>("count", &tileCount)
				.endNamespace()
				.beginNamespace("label")
				.addFunction<void, RefCountedPtr<Label>>("add", &addLabel)
				.addFunction<void, RefCountedPtr<Label>>("remove", &removeLabel)
				.addFunction<RefCountedPtr<Label>, ULONGLONG>("get", &getLabel)
				.addFunction<void>("reset", &resetLabels)
				.addFunction<int>("count", &labelCount)
				.endNamespace()
				.beginNamespace("sound")
				.addFunction<void, LPCSTR>("play", &playSound)
				.addFunction<void, LPCSTR>("loop", &loopSound)
				.addFunction<void>("reset", &resetSound)
				.endNamespace()
				.beginNamespace("cursor")
				.addVariable("position", &cursorPosition, false)
				.addFunction<void>("show", &showCursor)
				.addFunction<void>("hide", &hideCursor)
				.endNamespace()
				.addVariable("frames", &nFrames, false)
				.addVariable("camera", &camera, false)
				.addFunction<void, LPCSTR>("title", &title)
				.addFunction<void, Image>("icon", &icon)
				.addFunction<void, Transform>("view", &view)
				.addFunction<void>("destroy", &destroy)
				.beginNamespace("input")
				.addConstant("press", GLFW_PRESS)
				.addConstant("release", GLFW_RELEASE)
				.addFunction<bool, USHORT>("key", &Engine::key)
				.addFunction<bool, BYTE>("button", &Engine::button)
				.beginNamespace("keys")
				.addConstant("space", GLFW_KEY_SPACE)
				.addConstant("apostrophe", GLFW_KEY_APOSTROPHE)
				.addConstant("comma", GLFW_KEY_COMMA)
				.addConstant("minus", GLFW_KEY_MINUS)
				.addConstant("period", GLFW_KEY_PERIOD)
				.addConstant("slash", GLFW_KEY_SLASH)
				.addConstant("digit0", GLFW_KEY_0)
				.addConstant("digit1", GLFW_KEY_1)
				.addConstant("digit2", GLFW_KEY_2)
				.addConstant("digit3", GLFW_KEY_3)
				.addConstant("digit4", GLFW_KEY_4)
				.addConstant("digit5", GLFW_KEY_5)
				.addConstant("digit6", GLFW_KEY_6)
				.addConstant("digit7", GLFW_KEY_7)
				.addConstant("digit8", GLFW_KEY_8)
				.addConstant("digit9", GLFW_KEY_9)
				.addConstant("semicolon", GLFW_KEY_SEMICOLON)
				.addConstant("equal", GLFW_KEY_EQUAL)
				.addConstant("a", GLFW_KEY_A)
				.addConstant("b", GLFW_KEY_B)
				.addConstant("c", GLFW_KEY_C)
				.addConstant("d", GLFW_KEY_D)
				.addConstant("e", GLFW_KEY_E)
				.addConstant("f", GLFW_KEY_F)
				.addConstant("g", GLFW_KEY_G)
				.addConstant("h", GLFW_KEY_H)
				.addConstant("i", GLFW_KEY_I)
				.addConstant("j", GLFW_KEY_J)
				.addConstant("k", GLFW_KEY_K)
				.addConstant("l", GLFW_KEY_L)
				.addConstant("m", GLFW_KEY_M)
				.addConstant("n", GLFW_KEY_N)
				.addConstant("o", GLFW_KEY_O)
				.addConstant("p", GLFW_KEY_P)
				.addConstant("q", GLFW_KEY_Q)
				.addConstant("r", GLFW_KEY_R)
				.addConstant("s", GLFW_KEY_S)
				.addConstant("t", GLFW_KEY_T)
				.addConstant("u", GLFW_KEY_U)
				.addConstant("v", GLFW_KEY_V)
				.addConstant("w", GLFW_KEY_W)
				.addConstant("x", GLFW_KEY_X)
				.addConstant("y", GLFW_KEY_Y)
				.addConstant("z", GLFW_KEY_Z)
				.addConstant("leftBracket", GLFW_KEY_LEFT_BRACKET)
				.addConstant("backSlash", GLFW_KEY_BACKSLASH)
				.addConstant("rightBracket", GLFW_KEY_RIGHT_BRACKET)
				.addConstant("graveAccent", GLFW_KEY_GRAVE_ACCENT)
				.addConstant("world1", GLFW_KEY_WORLD_1)
				.addConstant("world2", GLFW_KEY_WORLD_2)
				.addConstant("escape", GLFW_KEY_ESCAPE)
				.addConstant("tab", GLFW_KEY_TAB)
				.addConstant("backSpace", GLFW_KEY_BACKSPACE)
				.addConstant("insert", GLFW_KEY_INSERT)
				.addConstant("arrowRight", GLFW_KEY_RIGHT)
				.addConstant("arrowLeft", GLFW_KEY_LEFT)
				.addConstant("arrowDown", GLFW_KEY_DOWN)
				.addConstant("arrowUp", GLFW_KEY_UP)
				.addConstant("pageUp", GLFW_KEY_PAGE_UP)
				.addConstant("pageDown", GLFW_KEY_PAGE_DOWN)
				.addConstant("home", GLFW_KEY_HOME)
				.addConstant("end", GLFW_KEY_END)
				.addConstant("capsLock", GLFW_KEY_CAPS_LOCK)
				.addConstant("scrollLock", GLFW_KEY_SCROLL_LOCK)
				.addConstant("numLock", GLFW_KEY_NUM_LOCK)
				.addConstant("printScreen", GLFW_KEY_PRINT_SCREEN)
				.addConstant("pause", GLFW_KEY_PAUSE)
				.addConstant("f1", GLFW_KEY_F1)
				.addConstant("f2", GLFW_KEY_F2)
				.addConstant("f3", GLFW_KEY_F3)
				.addConstant("f4", GLFW_KEY_F4)
				.addConstant("f5", GLFW_KEY_F5)
				.addConstant("f6", GLFW_KEY_F6)
				.addConstant("f7", GLFW_KEY_F7)
				.addConstant("f8", GLFW_KEY_F8)
				.addConstant("f9", GLFW_KEY_F9)
				.addConstant("f10", GLFW_KEY_F10)
				.addConstant("f11", GLFW_KEY_F11)
				.addConstant("f12", GLFW_KEY_F12)
				.addConstant("f13", GLFW_KEY_F13)
				.addConstant("f14", GLFW_KEY_F14)
				.addConstant("f15", GLFW_KEY_F15)
				.addConstant("f16", GLFW_KEY_F16)
				.addConstant("f17", GLFW_KEY_F17)
				.addConstant("f18", GLFW_KEY_F18)
				.addConstant("f19", GLFW_KEY_F19)
				.addConstant("f20", GLFW_KEY_F20)
				.addConstant("f21", GLFW_KEY_F21)
				.addConstant("f22", GLFW_KEY_F22)
				.addConstant("f23", GLFW_KEY_F23)
				.addConstant("f24", GLFW_KEY_F24)
				.addConstant("f25", GLFW_KEY_F25)
				.addConstant("keypad0", GLFW_KEY_KP_0)
				.addConstant("keypad1", GLFW_KEY_KP_1)
				.addConstant("keypad2", GLFW_KEY_KP_2)
				.addConstant("keypad3", GLFW_KEY_KP_3)
				.addConstant("keypad4", GLFW_KEY_KP_4)
				.addConstant("keypad5", GLFW_KEY_KP_5)
				.addConstant("keypad6", GLFW_KEY_KP_6)
				.addConstant("keypad7", GLFW_KEY_KP_7)
				.addConstant("keypad8", GLFW_KEY_KP_8)
				.addConstant("keypad9", GLFW_KEY_KP_9)
				.addConstant("keypadDecimal", GLFW_KEY_KP_DECIMAL)
				.addConstant("keypadDivide", GLFW_KEY_KP_DIVIDE)
				.addConstant("keypadMultiply", GLFW_KEY_KP_MULTIPLY)
				.addConstant("keypadSubstract", GLFW_KEY_KP_SUBTRACT)
				.addConstant("keypadAdd", GLFW_KEY_KP_ADD)
				.addConstant("keypadEnter", GLFW_KEY_KP_ENTER)
				.addConstant("keypadEqual", GLFW_KEY_KP_EQUAL)
				.addConstant("leftShift", GLFW_KEY_LEFT_SHIFT)
				.addConstant("leftControl", GLFW_KEY_LEFT_CONTROL)
				.addConstant("leftAlt", GLFW_KEY_LEFT_ALT)
				.addConstant("leftSuper", GLFW_KEY_LEFT_SUPER)
				.addConstant("rightShift", GLFW_KEY_RIGHT_SHIFT)
				.addConstant("rightControl", GLFW_KEY_RIGHT_CONTROL)
				.addConstant("rightAlt", GLFW_KEY_RIGHT_ALT)
				.addConstant("rightSuper", GLFW_KEY_RIGHT_SUPER)
				.addConstant("menu", GLFW_KEY_MENU)
				.endNamespace()
				.beginNamespace("buttons")
				.addConstant("left", GLFW_MOUSE_BUTTON_LEFT)
				.addConstant("right", GLFW_MOUSE_BUTTON_RIGHT)
				.addConstant("middle", GLFW_MOUSE_BUTTON_MIDDLE)
				.addConstant("x1", GLFW_MOUSE_BUTTON_4)
				.addConstant("x2", GLFW_MOUSE_BUTTON_5)
				.addConstant("x3", GLFW_MOUSE_BUTTON_6)
				.addConstant("x4", GLFW_MOUSE_BUTTON_7)
				.addConstant("x5", GLFW_MOUSE_BUTTON_8)
				.addConstant("hover", GLFW_MOUSE_BUTTON_LAST)
				.addConstant("scroll", GLFW_MOUSE_BUTTON_LAST + 1)
				.endNamespace()
				.endNamespace()
				.endNamespace()
				.endNamespace();
		}
	};

	HANDLE Engine::hMain = nullptr;
	bool Engine::bRunning = false;
	list<RefCountedPtr<Tile>> Engine::tiles = list<RefCountedPtr<Tile>>();
	list<RefCountedPtr<Label>> Engine::labels = list<RefCountedPtr<Label>>();
	float Engine::fps = 0.0f;
	float Engine::time = 0.0f;
	float Engine::deltaTime = 0.0f;
	float Engine::renderTime = 0.0f;
	Vector Engine::gravity = Vector(0.0f, -9.82f);
	Lua Engine::lua;
	GLFWwindow* Engine::glWindow = nullptr;
	bool Engine::lpKeys[GLFW_KEY_LAST + 1];
	bool Engine::lpButtons[GLFW_MOUSE_BUTTON_LAST + 2];
	Vector Engine::cursorPosition = Vector();
	ULONGLONG Engine::nFrames = 0;
	Transform Engine::camera = Transform();
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR nCmdLine, INT nCmdShow)
{
	if (!strlen(nCmdLine))
		if (std::filesystem::exists("Main.lua"))
			nCmdLine = (LPSTR)"Main.lua";
		else
			nCmdLine = (LPSTR)Flat::Explorer::select("Lua (*.lua)\0*.lua\0\0");

	Flat::Engine::start(nCmdLine);
	Flat::Engine::join();

	return 0;
}