function Initialize()
	timezone = ParseTimezone(SKIN:GetMeasure("Timezone"):GetStringValue())
	scale = SKIN:GetVariable("Scale")
	if not scale then scale = 1 end
	res = 10
end


function GetCurrentUTC()
	local time = os.date("*t")
	return table.concat({time.year, "-", time.month, "-", time.day})
end

function GetStartUTC(interval)
	local time = os.date("*t")
	time.hour = time.hour - interval
	return table.concat({time.year, "-", time.month, "-", time.day})
end

function ParseTimezone(timezone)
	local sign, hour, minute = timezone:match("(.)(%d%d)(%d%d)")
	sign = sign .. 1
	return {h = hour * sign, m = minute * sign}
end

function GetCurrentPrice()
	return dataTable[#dataTable].price
end

function GetNewData()
	local f = io.open(SKIN:GetVariable("CURRENTPATH") .. "DownloadFile\\Data.txt", 'r')
	local data = f:read("*a")
	dataTable = {}
	for t, p in data:gmatch("\"([%d%-%s:]+)\",([%d%.]+)") do
		table.insert(dataTable, {time = FormatTime(t, 0, 0), price = p*1})
	end
end

function DrawGraph(magickName, width, height)
	local totalData = #dataTable
	local max = 0
	local min = dataTable[totalData].price
	value = {}
	date = {}
	local step = 0
	for i = 1, totalData do
		if (dataTable[i].time.min == 30) then
			local curValue = dataTable[i].price
			if not curValue then curValue = 0 end
			if curValue > max then max = curValue end
			if curValue < min then min = curValue end
			value[#value+1] = curValue
			date[#date+1] = dataTable[i].time
		end
	end
	max,maxText = BetterCeil(max)
	min,minText = BetterFloor(min)
	if (max-min) ~= 0 then step = height / (max - min) end
	med = (max + min) / 2
--Draw paths
	pointerTable={}
	local path = '0,' .. ((med - value[1]) * step * scale)
	for k,v in pairs(value) do
		if k == #value then break end
		local y1,y2 = (med-v)*step, (med-value[k+1])*step
		local y0,y3 = (med-value[k == 1 and k or k-1])*step,(med-value[k ==#value-1 and k+1 or k+2])*step
		for i = 0,res do
			path = table.concat({
				path, '|LINETO ', ((k - 1)*width/#value+i*width/#value/res) * scale, ',', CubicInterpolate(y0,y1,y2,y3,i*1/res)*scale
			})
		end
	end
	SKIN:Bang('!SetOption', magickName, "GraphPath", path)
	SKIN:Bang('!CommandMeasure', magickName, "Update")
end

function CubicInterpolate(y0,y1,y2,y3,mu)
   local mu2,a0 = mu*mu, y3-y2-y0+y1
   local a1 = y0-y1-a0
   return (a0*mu*mu2+a1*mu2+(y2-y0)*mu+y1)
end

function BetterCeil(x,c)
	local temp = math.ceil(x)
	local num = 0
	while math.floor(temp/10) ~= 0 do
		temp = math.floor(temp/10)
		num = num + 1
	end
	if num < 3 then num = num
	elseif num == 3 then num = 2
	else num = num - num%3 end
	local text = ''
	if num < 3 then text = math.ceil( math.ceil(x) / math.pow(10,num) ) * math.pow(10,num)
	elseif num == 3 then text = (math.ceil( math.ceil(x) / math.pow(10,3) )..'K')
	elseif num == 6 then text = (math.ceil( math.ceil(x) / math.pow(10,6) )..'M')
	elseif num >= 9 then text = (math.ceil( math.ceil(x) / math.pow(10,9) )..'B')
	end
	return math.ceil( math.ceil(x) / math.pow(10,num) ) * math.pow(10,num),text
end

function BetterFloor(x)
	local temp = math.floor(x)
	local floored = math.floor(x)
	local num = 0
	while math.floor(temp/10) ~= 0 do
		temp = math.floor(temp/10)
		num = num + 1
	end
	if num < 3 then num = num
	elseif num == 3 then num = 2
	else num = num - num%3 end
	local text = ''
	if num < 3 then text = math.floor( math.floor(x) / math.pow(10,num) ) * math.pow(10,num)
	elseif num == 3 then text = (math.floor( math.floor(x) / math.pow(10,3) )..'K')
	elseif num == 6 then text = (math.floor( math.floor(x) / math.pow(10,6) )..'M')
	elseif num >= 9 then text = (math.floor( math.floor(x) / math.pow(10,9) )..'B')
	end
	return math.floor( math.floor(x) / math.pow(10,num) ) * math.pow(10,num),text
end

function FormatTime(time, GMTHour, GMTMinute)
	local y,m,d,h,minute,s = time:match('(%d+)%-(%d+)%-(%d+) (%d+):(%d+):(%d+)')
	return {year = y,
		month = m,
		day = d,
		hour = h + GMTHour,
		min = minute + GMTMinute,
		sec = s
	}
end