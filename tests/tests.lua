-- 27.08.2025 7:27:27
local GLOBAL_REF_TIME = 1756268847

require("chronology_lua")

_G.tests = {
	["date"] = {
		tests = {
			{
				name = "parse()",
				run = function()
					return date.parse("27.08.2025 7:27:27") == GLOBAL_REF_TIME
				end,
			},
			{
				name = "break_down()",
				run = function()
					local info = date.break_down(GLOBAL_REF_TIME)
					return info.sec == 27
						and info.min == 27
						and info.hour == 7
						and info.mday == 27
						and info.mon == 8
						and info.year == 2025
				end,
			},
		},
		run = function()
			local passed = 0
			local t = tests["date"].tests
			for _i, test in ipairs(t) do
				if test.run() then
					passed = passed + 1
					print("[TEST PASSED] date/" .. test.name)
				else
					print("[TEST FAILED] date/" .. test.name)
				end
			end
			return passed, #t
		end,
	},
	["cyclicacy"] = {
		tests = {
			{
				name = "does not repeat, happened before",
				recurrence = chrono.DOES_NOT_REPEAT,
				now = "27.08.2025 7:27:27",
				start = "04.08.2025 10:00",
				expect = "04.08.2025 10:00",
			},
			{
				name = "does not repeat, happens after",
				recurrence = chrono.DOES_NOT_REPEAT,
				now = "04.08.2025 7:27:27",
				start = "10.08.2025 10:00",
				expect = "10.08.2025 10:00",
			},
			{
				name = "repeats in seconds",
				recurrence = chrono.repeat_daily(1),
				now    = "10.08.2025 10:27:27",
				start  = "04.08.2025 10:00",
				expect = "11.08.2025 10:00",
			},
			{
				name = "repeats monthly, first wday, next month",
				recurrence = chrono.repeat_monthly(chrono.ON_FIRST_WDAY),
				now = "27.08.2025 7:27:27",
				start = "04.08.2025 10:00",
				expect = "01.09.2025 10:00",
			},
			{
				name = "repeats monthly, first wday, current month",
				recurrence = chrono.repeat_monthly(chrono.ON_FIRST_WDAY),
				now = "02.08.2025 7:27:27",
				start = "31.07.2025 10:00",
				expect = "07.08.2025 10:00",
			},
			{
				name = "repeats monthly, last wday, next month",
				recurrence = chrono.repeat_monthly(chrono.ON_LAST_WDAY),
				now = "27.08.2025 7:27:27",
				start = "25.08.2025 10:00",
				expect = "29.09.2025 10:00",
			},
			{
				name = "repeats monthly, last wday, current month",
				recurrence = chrono.repeat_monthly(chrono.ON_LAST_WDAY),
				now = "27.08.2025 7:27:27",
				start = "01.08.2025 10:00",
				expect = "29.08.2025 10:00",
			},
			{
				name = "repeats monthly, nth wday, next month",
				recurrence = chrono.repeat_monthly(chrono.ON_NTH_WDAY(2)),
				now = "27.08.2025 7:27:27",
				start = "04.08.2025 10:00",
				expect = "08.09.2025 10:00",
			},
			{
				name = "repeats monthly, nth wday, current month",
				recurrence = chrono.repeat_monthly(chrono.ON_NTH_WDAY(2)),
				now = "05.08.2025 7:27:27",
				start = "04.08.2025 10:00",
				expect = "11.08.2025 10:00",
			},
			{
				name = "repeats monthly, end-relative, next month",
				recurrence = chrono.repeat_monthly(chrono.END_RELATIVE(6)),
				now = "27.08.2025 7:27:27",
				start = "04.08.2025 10:00",
				expect = "25.09.2025 10:00",
			},
			{
				name = "repeats monthly, end-relative, current month",
				recurrence = chrono.repeat_monthly(chrono.END_RELATIVE(2)),
				now = "27.08.2025 7:27:27",
				start = "04.08.2025 10:00",
				expect = "30.08.2025 10:00",
			},
		},
		run = function(suite)
			local passed = 0
			local t = tests["cyclicacy"].tests
			for _i, test in ipairs(t) do
				test.now = date.parse(test.now)
				test.start = date.parse(test.start)
				test.expect = date.parse(test.expect)
				test.first = chrono.first_ocurrence(test.recurrence, test.start, test.now)
				if test.expect == test.first then
					passed = passed + 1
					print("[TEST PASSED] cyclicacy/" .. test.name)
				else
					print("[TEST FAILED] cyclicacy/" .. test.name)
				end
			end
			return passed, #t
		end,
	},
}

for name, data in pairs(tests) do
	local passed, total
	passed, total = data.run()
	print(string.format("[TEST TOTAL]: %s (%d/%d)", name, passed, total))
end
