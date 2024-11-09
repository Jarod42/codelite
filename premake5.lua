--[[
newoption {
	trigger = 'wx-root',
	value = 'path',
	description = 'Path of wxWidget root',
	category = 'script'
}

if _OPTIONS['wx-root'] ~= nil then
	WxRoot = _OPTIONS['wx-root']
end
print('WxRoot: ', WxRoot)
--]]
newoption {
	trigger = 'mysql-root',
	value = 'path',
	description = 'Path of wxWidget root',
	category = 'script',
	-- default = 'C:/Program Files/MySQL/MySQL Server 8.0'
}

newoption {
	trigger = 'nugetsource',
	value = 'string',
	description = 'url of nuget package source',
	category = 'script'
}

local FlexPath = 'c:/msys64/usr/bin'
local YaccPath = 'c:/msys64/usr/bin'
local FlexExe = path.join(FlexPath, 'flex')
local YaccExe = path.join(YaccPath, 'bison') .. ' --yacc'
local WxrcExe = 'wxrc'
-- local wxCrafterExe = 'bin/%{_ACTION}/%{cfg.platform}/%{cfg.buildcfg}/wxCrafter.exe'
local wxCrafterExe = 'C:/Program Files/wxCrafter/bin/wxCrafter.exe'


print('mysql-root: ', _OPTIONS['mysql-root'])
print('FlexPath: ', FlexPath)
print('YaccPath: ', YaccPath)

local function flex_command(prefix)
	prefix = prefix or '%{file.basename}_'
	if #prefix ~= 0 then
		prefix = '--prefix=' .. prefix
	end
	buildmessage ('flex -L -B --nounistd ' .. prefix .. ' -o %{file.basename}.cpp %{file.relpath}')
	buildoutputs { '%{!cfg.objdir}/%{file.basename}.cpp' }
	buildcommands { FlexExe .. ' --noline --batch --nounistd ' .. prefix .. ' -o %[%{!cfg.objdir}/%{file.basename}.cpp] %[%{!file.abspath}]' }
	compilebuildoutputs 'on'
end
local function flexpp_command(prefix, header)
	prefix = prefix or '%{file.basename}_'
	if #prefix ~= 0 then
		prefix = '--prefix=' .. prefix
	end
	buildmessage ('flex --c++  --noline --batch --nounistd --yyclass=flex::yyFlexLexer ' .. prefix .. ' --header-file=%[%{!cfg.objdir}/' .. header .. '] -o %{file.basename}.cpp %{file.relpath}')
	buildoutputs { '%{!cfg.objdir}/%{file.basename}.cpp' }
	buildcommands { FlexExe .. ' --c++ --noline --batch --nounistd --yyclass=flex::yyFlexLexer ' .. prefix .. ' -o %[%{!cfg.objdir}/%{file.basename}.cpp] %[%{!file.abspath}]' }
	compilebuildoutputs 'on'
end

local function yacc_command(prefix, header)
	prefix = prefix or '%{file.basename}_'
	local header_flag
	local outputs
	if header then
		header_flag = ' --header=%[%{!cfg.objdir}/' .. header .. ']'
		header = '%{!cfg.objdir}/' .. header
		outputs = { '%{!cfg.objdir}/%{file.basename}_parser.cpp', header}
	else
		header_flag = ''
		outputs = { '%{!cfg.objdir}/%{file.basename}_parser.cpp' }
	end
	-- yacc -dl  -t -v -p gdb_result_ gdb_result.y
	buildmessage('yacc --no-lines -p=%{file.basename}_ -o %{file.basename}_parser.cpp %{file.relpath}' .. ((header and '--header=' .. header ) or ''))
	buildoutputs(outputs)
	buildcommands { YaccExe .. ' --no-lines -p ' .. prefix .. ' -o %[%{!cfg.objdir}/%{file.basename}_parser.cpp] %[%{!file.abspath}]' .. header_flag } ---H %[%{!cfg.objdir}/%{file.basename}_parser.hpp]
	compilebuildoutputs 'on'
end

local function wxCrafterTarget(directory_name, wxcp, outputs)
    removefiles(table.translate(outputs, function(output)
        if output:endswith('hpp') then
            return directory_name .. '/' .. output:sub(1, -3)
        else
            return directory_name .. '/' .. output
        end
        end)) -- temporary
	includedirs { 'obj/%{_ACTION}/generated_source/' .. directory_name } -- for generated files from wxcp
	filter { 'files:' .. directory_name .. '/'.. wxcp }
		buildmessage('wxCrafter -g -o obj/%{_ACTION}/generated_source/' .. directory_name .. ' %{!file.abspath}')
		buildoutputs(table.translate(outputs, function(output) return 'obj/%{_ACTION}/generated_source/' .. directory_name .. '/'.. output end))
		buildcommands { '%[' .. wxCrafterExe .. ']' .. ' -g -o %[' .. path.getabsolute('.') .. '/obj/%{_ACTION}/generated_source/' .. directory_name .. '/] %[%{!file.abspath}]' }
		compilebuildoutputs 'on'
	filter {}
end

local function wxrc_command()
	-- wxrc -c -v -o resources.cpp resources.xrc
	buildmessage 'wxrc -c -v -o %{file.basename} %{file.relpath}'
	buildoutputs { '%{!cfg.objdir}/%{file.basename}.cpp' }
	buildcommands { WxrcExe .. ' -c -v -o %[%{!cfg.objdir}/%{file.basename}.cpp] %[%{!file.abspath}]' }
	compilebuildoutputs 'on'
end

local CODELITE_VERSION = '18.00.0'
local WXCRAFTER_VERSION = CODELITE_VERSION

local function template(file_in, dict, file_out)
	local content = io.readfile(file_in)
	for k,v in pairs(dict) do
		content = content:gsub(k, v)
	end
	os.mkdir(path.getdirectory(file_out))
	local f, err = os.writefile_ifnotequal(content, file_out)

	if (f == 0) then
		-- file not modified
	elseif (f < 0) then
		error(err, 0)
		return false
	elseif (f > 0) then
		print('Generated ' .. file_out)
	end
	return true
end

local function autoversion_h()
	return template(
		'LiteEditor/autoversion.h.in',
		{['@CODELITE_VERSION@'] = CODELITE_VERSION},
		path.join('solution', _ACTION, 'autoversion.h')
	)
end

local function wxcrafterversion_cpp()
	return template(
		'wxcrafter/wxcversion.cpp.in',
		{['@WXCRAFTER_VERSION@'] = WXCRAFTER_VERSION},
		path.join('solution', _ACTION, 'wxcversion.cpp')
	)
end

local have_autoversion_h = autoversion_h()
local have_wxcrafterversion_cpp = wxcrafterversion_cpp()

local function UsePCH()
	filter {'configurations:Release', 'platforms:x86' }
		forceincludes 'PCH/precompiled_header_release_32.h'
	filter {'configurations:Release', 'platforms:x64' }
		forceincludes 'PCH/precompiled_header_release.h'
	filter {'configurations:Debug' }
		forceincludes 'PCH/precompiled_header_dbg.h'
	filter {}
end


workspace 'codelite'
	location 'solution/%{_ACTION}'
	configurations { 'Debug', 'Release' }
	platforms { 'x86', 'x64' }

	cppdialect 'C++20'
	warnings 'Extra'
	externalanglebrackets 'On'
	externalwarnings 'Off'
	editorintegration 'On'
	clangtidy 'On'

	filter { 'action:vs*' }
if _OPTIONS['nugetsource'] ~= nil then
		nugetsource(_OPTIONS['nugetsource'])
end
		nuget {
			--'wxWidgets:3.2.2.1', --'wxWidgets.redist:3.2.2.1',-- native missing
			'wxWidgets:3.2.8', -- "official" package is broken, using Jarod42's' one :/
			'sqlite:3.8.4.2', 'sqlite.redist:3.8.4.2',
			-- mysql
			--'sqlite:3.13.0',
			-- 'libssh2-vc141_xp:1.8.2' -- or https://github.com/eroullit/libssh ?
		}
	filter {}

	objdir 'obj/%{_ACTION}' -- premake adds $(platformName)/$(configName)/$(AppName)
	targetdir 'bin/%{_ACTION}/%{cfg.platform}/%{cfg.buildcfg}'

	filter 'platforms:x86'
		architecture 'x86'
	filter 'platforms:x64'
		architecture 'x64'
	filter 'configurations:Debug'
		optimize 'Off'
		symbols 'On'
		defines 'DEBUG'
	filter 'configurations:Release'
		optimize 'On'
		symbols 'Off'
		defines { 'NDEBUG' }
	filter { 'configurations:Release', 'toolset:msc*' }
		symbols 'On' -- create pdb
	filter { 'toolset:msc*' }
		buildoptions { '/Zc:__cplusplus' } -- else __cplusplus would be 199711L
	filter 'system:windows'
		defines {'WIN32'}
		defines { '__WXMSW__' } -- #include <wx/platform.h> instead
	filter {}

	defines {'USE_SFTP=0'} -- =1 requires lib_ssh
	startproject 'LiteEditor'

	project 'Jarod42'
		kind 'ConsoleApp'
		files { 'Jarod42/**' }

		defines { 'JSON_USE_IMPLICIT_CONVERSIONS=0' } -- CMake: JSON_ImplicitConversions` to `OFF`
		externalincludedirs { 'submodules/assistant' }

-- ----------------------------------------------------------------------------
group 'submodules'
	project 'submodules'
		kind 'None'
		files { 'submodules/*.*' } -- CMakeList mainly
-- ----------------------------------------------------------------------------
	project 'assistant'
		kind 'StaticLib'
		files { 'submodules/assistant/assistant/**.*' }
		includedirs { 'submodules/assistant' }
		--defines { 'JSON_USE_IMPLICIT_CONVERSIONS=0' } -- CMake: JSON_ImplicitConversions` to `OFF`

local function UseAssistant()
	externalincludedirs { 'submodules/assistant' }
	-- defines { 'JSON_USE_IMPLICIT_CONVERSIONS=0' } -- CMake: JSON_ImplicitConversions` to `OFF`
	links { 'assistant' }
end
-- ----------------------------------------------------------------------------
	project 'cc-wrapper'
		kind 'ConsoleApp'
		files {'submodules/cc-wrapper/**'}
		externalincludedirs { 'submodules/cc-wrapper/tinyjson' }

		filter {'system:windows', 'files:submodules/cc-wrapper/src/winproc.cpp'}
			buildaction 'None'
		filter {'system:not windows', 'files:submodules/cc-wrapper/src/unixproc.cpp'}
			buildaction 'None'
		filter {}

-- ----------------------------------------------------------------------------
	project 'doctest'
		kind 'None'
		files {'submodules/doctest/**'}

function UseDocTest()
	externalincludedirs {'submodules/doctest/doctest'}
end
-- ----------------------------------------------------------------------------
	project 'dtl'
		kind 'None' -- header library
		files {'submodules/dtl/**'}

local function UseDtl()
	externalincludedirs { 'submodules/dtl' }
end

-- ----------------------------------------------------------------------------
	project 'hunspell'
		kind 'StaticLib' -- or 'SharedLib'
		files { 'submodules/hunspell/**' }
		externalincludedirs { 'submodules/hunspell/src' }
	
		filter 'kind:StaticLib'
			defines 'HUNSPELL_STATIC'
		filter 'kind:SharedLib'
			defines 'BUILDING_LIBHUNSPELL'
		filter 'files:submodules/hunspell/src/parsers/** or submodules/hunspell/src/tools/**'
			buildaction 'None'
		filter 'toolset:msc*'
			disablewarnings {
				'4244', -- '=': conversion from '%type1' to '%type2', possible loss of data
				--'4251', -- '%class::%func': '%type' need to have dll-interface to be used by clients of '%class'
				'4267', -- '+=': conversion from '%type1' to '%type2', possible loss of data
				'4996', -- DEPRECATIONs
			}

local function UseHunspell(libKind)
	libKind = libKind or 'StaticLib'
	externalincludedirs { 'hunspell/src' }
	links 'hunspell'
	if libKind == 'StaticLib' then
		defines 'HUNSPELL_STATIC'
	end
end
-- ----------------------------------------------------------------------------
	project 'libssh'
		kind 'None'
		files {'submodules/libssh/**'}
-- ----------------------------------------------------------------------------
	project 'lua'
		kind 'StaticLib'
		files { 'submodules/lua/**' }
		removefiles {
			'submodules/lua/lua.c', -- for lua.exe
			'submodules/lua/onelua.c', -- one file build
			'submodules/lua/testes/**',
		}
local function UseLua()
	externalincludedirs { 'submodules/lua' }
	links { 'lua' }
end
-- ----------------------------------------------------------------------------
	project 'LuaBridge'
		kind 'None' -- header library
		files {'submodules/LuaBridge/**'}
local function UseLuaBridge()
	externalincludedirs { 'submodules/LuaBridge/Source' }
end
-- ----------------------------------------------------------------------------
	project 'openssl-cmake'
		kind 'None'
		files {'submodules/openssl-cmake/**'}
-- ----------------------------------------------------------------------------
group 'submodules'
-- ----------------------------------------------------------------------------
	project 'yaml-cpp'
		kind 'SharedLib'
		files {'submodules/yaml-cpp/**'}

		includedirs { 'submodules/yaml-cpp/include' }

		filter { 'files:not submodules/yaml-cpp/src/**' }
			buildaction 'None'

		filter { 'kind:StaticLib' }
			defines { 'YAML_CPP_STATIC_DEFINE' }
		filter { 'kind:SharedLib' }
			defines { 'yaml_cpp_EXPORTS' }

		filter 'toolset:msc*'
			disablewarnings {
				'4244', -- '=': conversion from '%type' to '%type2', possible loss of data
				'4251', -- '$var': class '%type' needs to have dll-interface to de used by client of class '$type2'
				'4275', -- non dll-interface class '$type' used as base class of dll-interface class '$type2'
				'4702', -- unreachable code
			}
		filter {}

local function UseYamlCpp()
	includedirs { 'submodules/yaml-cpp/include' }
	links { 'yaml-cpp' }
end

-- ----------------------------------------------------------------------------
	project 'wxsqlite3'
		kind 'SharedLib'
		files { 'sdk/wxsqlite3/**' } -- TODO: Remove
		removefiles { 'sdk/wxsqlite3/**.rc' } -- TODO: Remove
		includedirs { 'sdk/wxsqlite3/include' } -- TODO: Remove
		--files { 'submodules/wxsqlite3/**' }
		--removefiles { 'submodules/wxsqlite3/**.rc' }
		--includedirs { 'submodules/wxsqlite3/include' }
		defines 'WXMAKINGDLL_WXSQLITE3'

		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4702', -- unreachable code
			}
		filter {}

local function UseWxSqlite3()
	includedirs { 'sdk/wxsqlite3/include' } -- TODO: Remove
	--includedirs { 'submodules/wxsqlite3/include' }
	defines { 'WXUSINGDLL_WXSQLITE3' }
	links { 'wxsqlite3' }
end

-- ----------------------------------------------------------------------------
	project 'zlib'
		kind 'None'
		files {'submodules/zlib/**'}
-- ----------------------------------------------------------------------------
group 'submodules/lexilla'
-- ----------------------------------------------------------------------------
	project 'lexilla_lexers'
		kind 'StaticLib'
		files { 'submodules/lexilla/lexers/**' }
		includedirs { 'submodules/lexilla/include', 'submodules/lexilla/include/scintilla', 'submodules/lexilla/lexlib' }
		links { 'lexilla_lexlib' }
-- ----------------------------------------------------------------------------
	project 'lexilla_lexlib'
		kind 'StaticLib'
		files { 'submodules/lexilla/lexlib/**' }
		includedirs { 'submodules/lexilla/include', 'submodules/lexilla/include/scintilla', 'submodules/lexilla/lexlib' }
-- ----------------------------------------------------------------------------
	project 'lexilla_other'
		kind 'None'
		files { 'submodules/lexilla/**' }
		removefiles { 'submodules/lexilla/lexers/**', 'submodules/lexilla/lexlib/**' }

local function UseLexilla()
	externalincludedirs { 'submodules/lexilla/include', 'submodules/lexilla/include/scintilla', 'submodules/lexilla/lexlib' }
	links { 'lexilla_lexers' }
end
-- ----------------------------------------------------------------------------
group 'submodules/wx-config-msys2'
-- ----------------------------------------------------------------------------
	project 'utilslib'
		kind 'StaticLib'
		cppdialect 'c++20'
		files { 'submodules/wx-config-msys2/*', 'submodules/wx-config-msys2/src/*' }
		removefiles { 'submodules/wx-config-msys2/src/wx-config.cpp', 'submodules/wx-config-msys2/src/wx-config-msys2.cpp' }
		filter 'toolset:msc*'
			disablewarnings {
				'4456', -- declaration of '$var' hides previous declaration
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'wx-config'
		kind 'ConsoleApp'
		cppdialect 'c++20'
		files { 'submodules/wx-config-msys2/src/wx-config.cpp' }
		links { 'utilslib' }
-- ----------------------------------------------------------------------------
	project 'wx-config-msys2'
		kind 'ConsoleApp'
		cppdialect 'c++20'
		files { 'submodules/wx-config-msys2/src/wx-config-msys2.cpp' }
		links { 'utilslib' }
-- ----------------------------------------------------------------------------
group 'submodules/wxdap'
-- ----------------------------------------------------------------------------
	project 'wxdap'
		kind 'SharedLib'
		files { 'submodules/wxdap/dap/**.*', 'submodules/wxdap/*.*' }
			defines { 'WXMAKINGDLL_DAP' }
		filter 'system:windows'
			links { 'ws2_32' }
		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4101', -- '$var': unreferenced local variable
				'4189', -- '$var': local variable is initialized but not referenced
				'4244', -- '$op': conversion from '$type1' to '$type2', possible loss of data
				'4245', -- 'initializing': conversion from '$type1' to '$type2', signed/unsigned mismatch
				'4267', -- '$op': conversion from '$type1' to '$type2', possible loss of data
				'4458', -- declaration of '$var' hides class member
				'4702', -- unreachable code
				'4706', -- assignment within conditional expression
				'4996', -- DEPRECATIONs
			}
		filter {}
local function UseWxDap()
	links 'wxdap'
	includedirs 'submodules/wxdap'
	defines { 'WXUSINGDLL_DAP' }
end
	project 'dbgcli'
		kind 'WindowedApp'
		files { 'submodules/wxdap/dbgcli/**.*' }
		UseWxDap()

		filter 'files:**.rc'
			buildaction 'None'
		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4458', -- declaration of '$var' hides class member
			}
		filter {}
	project 'dap_test'
		kind 'ConsoleApp'
		files { 'submodules/wxdap/tests/**.*' }
		UseWxDap()
-- ----------------------------------------------------------------------------
group 'submodules/wxTerminalEmulator'
-- ----------------------------------------------------------------------------
	project 'wxTerminalLib'
		kind 'StaticLib'
		files {'submodules/wxTerminalEmulator/**'}
		removefiles { 'submodules/wxTerminalEmulator/main.cpp', 'submodules/wxTerminalEmulator/wx.rc' }

		defines { 'wxDEBUG_LEVEL=0' }
		filter { 'system:windows', 'files:submodules/wxTerminalEmulator/pty_backend_posix.cpp' }
			buildaction 'None'
		filter { 'system:not windows', 'files:submodules/wxTerminalEmulator/pty_backend_windows.cpp' }
			buildaction 'None'
		filter {}
function UseWxTerminal()
		externalincludedirs { 'submodules/wxTerminalEmulator' }
end
-- ----------------------------------------------------------------------------
	project 'wxTerminalEmulator'
		kind 'WindowedApp'
		files {
			'submodules/wxTerminalEmulator/main.cpp',
			'submodules/wxTerminalEmulator/wx.rc'
		}
		links { 'wxTerminalLib' }
		UseWxTerminal()
		filter { 'files:submodules/wxTerminalEmulator/wx.rc' }
			buildaction 'None'
		filter { 'system:windows' }
			links { 'shell32', 'ole32', 'user32' }
		filter {}
-- ----------------------------------------------------------------------------
group 'sdk'
-- ----------------------------------------------------------------------------
	project 'databaselayersqlite'
		kind 'SharedLib'
		--files { 'sdk/databaselayer/**.*' }
		files {
			'sdk/databaselayer/src/dblayer/Sqlite*.cpp', -- sqlite3
			'sdk/databaselayer/src/dblayer/Mysql*.cpp', -- MySql
			'sdk/databaselayer/src/dblayer/Postgres*.cpp', -- Postgres
			'sdk/databaselayer/src/dblayer/Database*.cpp',
			'sdk/databaselayer/src/dblayer/Prepared*.cpp',
			'sdk/databaselayer/src/*.*', -- project files
		}
		files {'sdk/databaselayer/**.h', 'sdk/databaselayer/**.txt'}
		includedirs { 'sdk/databaselayer/include/wx/dblayer/include' }
		defines 'WXMAKINGDLL_DATABASELAYER'
		UseWxSqlite3()

if _OPTIONS['mysql-root'] then
		includedirs 'C:/msys64/clang64/include/mysql'
		--includedirs(path.join(_OPTIONS['mysql-root'], 'include'))
		libdirs(path.join(_OPTIONS['mysql-root'], 'lib'))
		links { 'mysql' }
end
		--filter {'files:sdk/databaselayer/src/dblayer/Mysql*.cpp'}
		--	buildaction 'None' -- requires <mysql.h>
		filter {'files:sdk/databaselayer/src/dblayer/Postgres*.cpp'}
			buildaction 'None'

		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
			}
		filter {}

local function UseWxDatabaseLayer()
	includedirs { 'sdk/databaselayer/include' }
	links { 'databaselayersqlite' }
	defines 'WXUSINGDLL_DATABASELAYER'
	-- defines { 'DBL_USE_MYSQL=1' } -- requires libmysqlclient
end

-- ----------------------------------------------------------------------------
group ''
-- ----------------------------------------------------------------------------
  project 'Interfaces'
	kind 'Makefile' -- Headers only
	files { 'Interfaces/**.*' }

	includedirs { 'Plugin', 'submodules/wxsqlite3/include', 'Codelite', 'CodeLite/ssh', 'PCH', 'Interfaces' }
	defines { 'WXUSINGDLL_CL', 'WXUSINGDLL_WXSQLITE3' }

-- ----------------------------------------------------------------------------
	project 'libcodelite'
		kind 'SharedLib'
		files { 'Codelite/**.*' }
		includedirs {
			'CodeLite',
			'CodeLite/ssh',
			'Interfaces',
			'CxxParser'
		}

		defines 'wxUSE_GUI' -- mostly for generated for clprogressdlgbase.h
		defines 'WXMAKINGDLL_CL'
		links { 'LibCxxParser' }
		UseAssistant()
		UseWxSqlite3()

		filter 'system:windows'
			links { 'ws2_32' }
		--removefiles { 'Codelite/Cxx/lex.yy.cpp', 'Codelite/Cxx/FlexLexer.h'} -- generated by cpp5.l
		--filter 'files:Codelite/Cxx/cpp5.l'
		--	flexpp_command('', 'FlexLexer.h')
		filter 'files:Codelite/Cxx/CxxScanner.l'
			flex_command('')
		filter 'files:Codelite/PHP/PhpLexer.l'
			flex_command('php')
		filter 'files:Codelite/Cxx/include_finder.l'
			flex_command('inclf_')
		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4101', -- '$var': unreferenced local variable
				'4102', -- '$label': unreferenced label
				'4244', -- '=': conversion from '%type' to '%type2', possible loss of data
				'4245', -- 'initializing': conversion from '%type' to '%type2', signed/unsigned mismatch
				'4251', -- '$class::$member': class '$type' need to have dll-interface to be used by client of class '$class'
				'4267', -- '+=': conversion from '%type' to '%type2', possible loss of data
				'4390', -- ';': empty controlled statement found; is this the intent?
				'4456', -- declaration of '$var' hides previous local declaration
				'4457', -- declaration of '$var' hides function parameter
				'4458', -- declaration of '$var' hides class member
				'4459', -- declaration of '$var' hides global declaration
				'4505', -- '$var': unreferenced function with internal linkage has been removed
				'4706', -- assignment in conditional expression
				'4996', -- '$func_or_var': This function or variable may be is unsafe. Consider using $func_or_var2 instead. To disble deprecation use _CRT_SECURE_NO_WARNINGS.
			}
		filter {}

local function UseLibCodelite()
	UseAssistant()
	UseWxSqlite3()
	includedirs { 'Plugin', 'Plugin/aui', 'Codelite', 'CodeLite/ssh', 'CxxParser', 'PCH', 'Interfaces' }
	defines { 'WXUSINGDLL_CL' }
	links { 'libcodelite' }

	filter 'toolset:msc*'
		disablewarnings {
			'4100', -- '$var': unreferenced formal parameter
			'4458', -- declaration of '$var' hides class member
		}
	filter {}
end

-- ----------------------------------------------------------------------------
	project 'Plugin'
		kind 'SharedLib'
		files { 'Plugin/**.*' }

		filter { 'system:linux' }
			removefiles { 'Plugin/macos/**.*', 'Plugin/msw/**.*' }
		filter { 'system:macos' }
			removefiles { 'Plugin/gtk/**.*', 'Plugin/msw/**.*' }
		filter { 'system:windows' }
			removefiles { 'Plugin/gtk/**.*', 'Plugin/macos/**.*' }
		filter {}

		includedirs {
			'Plugin',
			'ThemeImporters',
			--'wxTerminalCtrl',
			'Interfaces'
		}
		links { 'wxTerminalLib' }
		UseDtl()
		UseLexilla()
		UseLibCodelite()
		UseLua()
		UseLuaBridge()
		UseYamlCpp()
		UseWxTerminal()

		defines { 'WXMAKINGDLL_SDK' }

		filter 'toolset:msc*'
			disablewarnings {
				--'4066', -- character beyond first in wide-character constant ignored
				--'4099', -- '$type': type name first seen using 'struct' now seen using 'class'
				--'4101', -- '$var': unreferenced local variable
				--'4189', -- '$var': local variable is initialized but not referenced
				'4244', -- '=': conversion from '%type' to '%type2', possible loss of data
				--'4245', -- 'initializing': conversion from '%type' to '%type2', signed/unsigned mismatch
				'4267', -- '+=': conversion from '%type' to '%type2', possible loss of data
				--'4310', -- cast truncates constant value
				--'4456', -- declaration of '$var' hides previous local declaration
				'4457', -- declaration of '$var' hides function parameter
				--'4459', -- declaration of '$var' hides global declaration
				--'4706', -- assignment in conditional expression
				'4996', -- '$func_or_var': This function or variable may be is unsafe. Consider using $func_or_var2 instead. To disble deprecation use _CRT_SECURE_NO_WARNINGS.
			}
		filter {}

local function UsePlugin()
	UseLibCodelite()
	UseLua()
	UseLuaBridge()
	UseWxTerminal()
	includedirs { 'Plugin', 'Plugin/aui', 'Interfaces' }
	defines { 'WXUSINGDLL_SDK' }
	links { 'plugin' }
end

-- ----------------------------------------------------------------------------
	project 'ALL_FILES'
		kind 'None'

		files {
			'art/**.*',
			'cmake/**.*',
			'codelite-icons/**.*',
			'codelite-icons-dark/**.*',
			'codelite-icons-fresh-farm/**.*',
			'docs/**.*',
			'formbuilder/**.*',
			'icons/**.*',
			'InnoSetup/**.*',
			'PCH/**.*',
			'Runtime/**.*',
			'scripts/**.*',
			'svgs/**.*',
			'tools/**.*',
			'translations/**.*',
			'weekly/**.*',
			'*.*'
		}

-- ----------------------------------------------------------------------------
group 'Plugins/codelitephp'
-- ----------------------------------------------------------------------------
	project 'codelitephp'
		kind 'SharedLib'
		files { 'codelitephp/*.*', 'codelitephp/bin/**.*', 'codelitephp/php-plugin/**.*', 'codelitephp/resources/**.*' }
		includedirs { 'codelitephp/PHPParser' }
		links { 'PHPParser' }
		wholearchive { 'PHPParser' }
		UsePlugin()
		wxCrafterTarget('codelitephp/php-plugin', 'new_class.wxcp', {'new_class.cpp', 'new_class.hpp', 'new_class_php-plugin_bitmaps.cpp'})
		wxCrafterTarget('codelitephp/php-plugin', 'php_ui.wxcp', {'php_ui.cpp', 'php_ui.hpp', 'php_ui_php-plugin_bitmaps.cpp'})
		filter 'toolset:msc*'
			disablewarnings {
			--'4702', -- unreachable code
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'PHPParser'
		kind 'StaticLib'
		files { 'codelitephp/PHPParser/**.*' }
		includedirs { 'Plugin', 'Plugin/aui', 'submodules/cJSON', 'submodules/assistant', 'submodules/wxsqlite3/include', 'Codelite', 'CodeLite/ssh', 'PCH', 'Interfaces' }

		filter 'toolset:msc*'
			disablewarnings {
			'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'PHPUnitTests'
		kind 'ConsoleApp'
		files { 'codelitephp/PHPParserUnitTests/**.*' }
		links { 'PHPParser' }
		UseDocTest()
		UseLibCodelite()
-- ----------------------------------------------------------------------------
group 'Plugins/Debugger'
-- ----------------------------------------------------------------------------
	project 'Debugger'
		kind 'SharedLib'
		files { 'Debugger/**.*' }
		includedirs { 'gdbparser' }
		--includedirs 'obj/%{_ACTION}/%{cfg.platform}/%{cfg.buildcfg}/libgdbparser' -- generated files
		UsePlugin()
		links { 'libgdbparser' }
		filter 'toolset:msc*'
			disablewarnings {
				'4102', -- '$label': unreferenced label
				'4190', -- '$func' has C-linkage specified, but returns UDT '$func' which is incompatible with C
				'4244', -- '=': conversion from '%type' to '%type2', possible loss of data
				'4706', -- assignment in conditional expression
			}
		filter {}

-- ----------------------------------------------------------------------------
	project 'libgdbparser'
		kind 'StaticLib'
		files { 'gdbparser/**.l' }
		files { 'gdbparser/**.y' }
		files { 'gdbparser/gdb_parser_incl.h' }

		includedirs { 'gdbparser' }
		includedirs { '%{cfg.objdir}' } -- generated files

		filter 'files:gdbparser/gdb_result.l'
			flex_command()
		filter 'files:gdbparser/gdb_result.y'
			yacc_command(nil, 'gdb_result_parser.h')
		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4102', -- '$label': unreferenced label
				'4244', -- '%op': conversion from '%type1' to '%type2', possible loss of data
				'4267', -- '%op': conversion from '%type1' to '%type2', possible loss of data
				'4706', -- assignment within conditional expression
				'4715', -- '%func': not all control paths return a value
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'gdbparser'
		kind 'ConsoleApp'
		files { 'gdbparser/main.cpp' }
		files { 'gdbparser/**.project' }
		files { 'gdbparser/**.workspace' }
		files { 'gdbparser/**.txt' }

		links { 'libgdbparser' }
		includedirs { 'gdbparser' }
		includedirs 'obj/%{_ACTION}/%{cfg.platform}/%{cfg.buildcfg}/libgdbparser' -- generated files

		debugdir 'gdbparser'

		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4102', -- '$label': unreferenced label
				'4244', -- '%op': conversion from '%type1' to '%type2', possible loss of data
				'4267', -- '%op': conversion from '%type1' to '%type2', possible loss of data
				'4706', -- assignment within conditional expression
				'4715', -- '%func': not all control paths return a value
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
group 'Plugins'
-- ----------------------------------------------------------------------------
	project 'abbreviation'
		kind 'SharedLib'
		files { 'abbreviation/**.*' }
		UsePlugin()
		wxCrafterTarget('abbreviation', 'abbreviationssettingsbase.wxcp', {'abbreviationssettingsbase.cpp', 'abbreviationssettingsbase.hpp', 'abbreviationssettingsbase_abbreviation_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'AutoSave'
		kind 'SharedLib'
		files { 'AutoSave/**.*' }
		UsePlugin()
		wxCrafterTarget('AutoSave', 'AutoSaveUI.wxcp', {'AutoSaveUI.cpp', 'AutoSaveUI.hpp', 'AutoSaveUI_autosave_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'CallGraph'
		kind 'SharedLib'
		files { 'CallGraph/**.*' }
		UsePlugin()
		filter 'toolset:msc*'
			disablewarnings {
				'4305', -- '$op': truncation from '$type1' to '$type2'
				'4996', -- '$func_or_var': This function or variable may be is unsafe. Consider using $func_or_var2 instead. To disble deprecation use _CRT_SECURE_NO_WARNINGS.
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'CMakePlugin'
		kind 'SharedLib'
		files { 'CMakePlugin/**.*' }
		UsePlugin()
		wxCrafterTarget('CMakePlugin', 'CMakePlugin.wxcp', {'CMakePluginUi.cpp', 'CMakePluginUi.hpp', 'CMakePluginUi_bitmaps.cpp'})
		filter 'toolset:msc*'
			disablewarnings {
				'4457', -- declaration of '$var' hides function parameter
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'CodeFormatter'
		kind 'SharedLib'
		files { 'CodeFormatter/**.*' }
		UsePlugin()
		wxCrafterTarget('CodeFormatter', 'codeformatterdlg.wxcp', {'codeformatterdlg.cpp', 'codeformatterdlg.hpp', 'codeformatterdlg_codeformatter_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'codelite_vim'
		kind 'SharedLib'
		files { 'codelite_vim/**.*' }
		UsePlugin()
		wxCrafterTarget('codelite_vim', 'VIM.wxcp', {'VIM.cpp', 'VIM.hpp', 'VIM_codelite_vim_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'CodeLiteDiff'
		kind 'SharedLib'
		files { 'CodeLiteDiff/**.*' }
		UsePlugin()
		wxCrafterTarget('CodeLiteDiff', 'CodeLiteDiff_UI.wxcp', {'CodeLiteDiff_UI.cpp', 'CodeLiteDiff_UI.hpp', 'CodeLiteDiff_UI_codelitediff_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'ContinuousBuild'
		kind 'SharedLib'
		files { 'ContinuousBuild/**.*' }
		UsePlugin()
		wxCrafterTarget('ContinuousBuild', 'continuousbuildbasepane.wxcp', {'continuousbuildbasepane.cpp', 'continuousbuildbasepane.hpp', 'continuousbuildbasepane_continuousbuild_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'Copyright'
		kind 'SharedLib'
		files { 'Copyright/**.*' }
		UsePlugin()
-- ----------------------------------------------------------------------------
	project 'cppchecker'
		kind 'SharedLib'
		files { 'cppchecker/**.*' }
		UsePlugin()
		wxCrafterTarget('cppchecker', 'cppcheckersettingsdlg.wxcp', {'cppcheckersettingsdlg.cpp', 'cppcheckersettingsdlg.hpp', 'cppcheckersettingsdlg_cppchecker_bitmaps.cpp'})
		filter 'toolset:msc*'
			disablewarnings {
				'4996', -- 'wxPATH_NORM_ALL': specify the wanted flags explicitly to avoid surprises
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'cscope'
		kind 'SharedLib'
		files { 'cscope/**.*' }
		UsePlugin()
		wxCrafterTarget('cscope', 'CscopeTabBase.wxcp', {'CscopeTabBase.cpp', 'CscopeTabBase.hpp', 'CscopeTabBase_scope_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'DatabaseExplorer'
		kind 'SharedLib'
		files { 'DatabaseExplorer/**.*' }
		UseWxDatabaseLayer()
		UsePlugin()
		wxCrafterTarget('DatabaseExplorer', 'GUI.wxcp', {'GUI.cpp', 'GUI.hpp', 'GUI_databaseexplorer_bitmaps.cpp'})
		defines {
			'DBL_USE_SQLITE',
			--'DBL_USE_MYSQL',
			--'DBL_USE_POSTGRES'
		}
		filter { 'files:DatabaseExplorer/resource.rc' }
			buildaction 'None'
		filter {}
-- ----------------------------------------------------------------------------
	project 'DebugAdapterClient'
		kind 'SharedLib'
		files { 'DebugAdapterClient/**.*' }
		UsePlugin()
		UseWxDap()
		wxCrafterTarget('DebugAdapterClient', 'UI.wxcp', {'UI.cpp', 'UI.hpp', 'UI_lldbdebugger_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'Docker'
		kind 'SharedLib'
		files { 'Docker/**.*' }
		UsePlugin()
		wxCrafterTarget('Docker', 'UI.wxcp', {'UI.cpp', 'UI.hpp', 'UI_docker_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'EditorConfigPlugin'
		kind 'SharedLib'
		files { 'EditorConfigPlugin/**.*' }
		UsePlugin()
		wxCrafterTarget('EditorConfigPlugin', 'EditorConfigUI.wxcp', {'EditorConfigUI.cpp', 'EditorConfigUI.hpp', 'EditorConfigUI_editorconfigplugin_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'EOSWiki'
		kind 'SharedLib'
		files { 'EOSWiki/**.*' }
		UsePlugin()
		wxCrafterTarget('EOSWiki', 'eosUI.wxcp', {'eosUI.cpp', 'eosUI.hpp', 'eosUI_eoswiki_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'ExternalTools'
		kind 'SharedLib'
		files { 'ExternalTools/**.*' }
		UsePlugin()
-- ----------------------------------------------------------------------------
	project 'git'
		kind 'SharedLib'
		files { 'git/**.*' }
		UsePlugin()
		wxCrafterTarget('git', 'gitui.wxcp', {'gitui.cpp', 'gitui.hpp', 'gitui_git_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'Gizmos'
		kind 'SharedLib'
		files { 'Gizmos/**.*' }
		UsePlugin()
		wxCrafterTarget('Gizmos', 'gizmos.wxcp', {'gizmos_base.cpp', 'gizmos_base.hpp', 'gizmos_gizmos_bitmaps.cpp'})
		filter 'toolset:msc*'
			disablewarnings {
				'4996', -- '$func': specify the wanted flags explicitly to avoid surprises
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'HelpPlugin'
		kind 'SharedLib'
		files { 'HelpPlugin/**.*' }
		UsePlugin()
		wxCrafterTarget('HelpPlugin', 'HelpPluginUI.wxcp', {'HelpPluginUI.cpp', 'HelpPluginUI.hpp', 'HelpPluginUI_helpplugin_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'LanguageServer'
		kind 'SharedLib'
		files { 'LanguageServer/**.*' }
		includedirs { 'LanguageServer' }
		UsePlugin()
-- ----------------------------------------------------------------------------
	project 'MacBundler'
		kind 'SharedLib'
		files { 'MacBundler/**.*' }
		UsePlugin()

		filter 'toolset:msc*'
			defines { 'and=&&', 'not=!', 'or=||' } -- MSVC buggy?
		filter {}
-- ----------------------------------------------------------------------------
	project 'MemCheck' -- disabled on macOS
		kind 'SharedLib'
		files { 'MemCheck/**.*' }
		UsePCH()
		UsePlugin()
		wxCrafterTarget('MemCheck', 'memcheckui.wxcp', {'memcheckui.cpp', 'memcheckui.hpp', 'memcheckui_bitmaps.cpp'})

		filter 'toolset:msc*'
			disablewarnings {
				'4245', -- 'return': conversion from '$type1' to '$type2'
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'Outline'
		kind 'SharedLib'
		files { 'Outline/**.*' }
		UsePlugin()
		wxCrafterTarget('Outline', 'wxcrafter.wxcp', {'wxcrafter.cpp', 'wxcrafter.hpp', 'wxcrafter_outline_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'PHPLint'
		kind 'SharedLib'
		files { 'PHPLint/**.*' }
		UsePlugin()
		wxCrafterTarget('PHPLint', 'phplintdlg.wxcp', {'phplintdlgbase.cpp', 'phplintdlgbase.hpp', 'phplintdlg_phplint_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'PHPRefactoring'
		kind 'SharedLib'
		files { 'PHPRefactoring/**.*' }
		UsePlugin()
		wxCrafterTarget('PHPRefactoring', 'phprefactoringdlgbase.wxcp', {'phprefactoringdlgbase.cpp', 'phprefactoringdlgbase.hpp', 'phprefactoringdlg_phprefactoring_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'QmakePlugin'
		kind 'SharedLib'
		files { 'QmakePlugin/**.*' }
		UsePlugin()
		wxCrafterTarget('QmakePlugin', 'NewQtProj.wxcp', {'NewQtProj.cpp', 'NewQtProj.hpp', 'NewQtProj_qmakeplugin_bitmaps.cpp'})
		wxCrafterTarget('QmakePlugin', 'qmakesettingsbasedlg.wxcp', {'qmakesettingsbasedlg.cpp', 'qmakesettingsbasedlg.hpp', 'qmakesettingsbasedlg_qmakeplugin_bitmaps.cpp'})
		wxCrafterTarget('QmakePlugin', 'qmaketabbase.wxcp', {'qmaketabbase.cpp', 'qmaketabbase.hpp', 'qmaketabbase_qmakeplugin_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
-- [[ Requires USE_SFTP
	project 'Remoty'
		kind 'Makefile'
		--kind 'SharedLib'
		files { 'Remoty/**.*' }
		UsePCH()
		UsePlugin()
--]]
-- ----------------------------------------------------------------------------
	project 'Rust'
		kind 'SharedLib'
		files { 'Rust/**.*' }
		UsePlugin()
-- ----------------------------------------------------------------------------
-- [[ Requires USE_SFTP
	project 'SFTP'
		kind 'Makefile'
		--kind 'SharedLib'
		files { 'SFTP/**.*' }
		UsePCH()
		UsePlugin()
		wxCrafterTarget('SFTP', 'UI.wxcp', {'UI.cpp', 'UI.hpp', 'UI_sftp_bitmaps.cpp'})
--]]
-- ----------------------------------------------------------------------------
	project 'SmartCompletion'
		kind 'SharedLib'
		files { 'SmartCompletion/**.*' }
		UsePlugin()
		wxCrafterTarget('SmartCompletion', 'SmartCompletionsUI.wxcp', {'SmartCompletionsUI.cpp', 'SmartCompletionsUI.hpp', 'SmartCompletionsUI_smartcompletion_bitmaps.cpp'})
-- ----------------------------------------------------------------------------
	project 'SnipWiz'
		kind 'SharedLib'
		files { 'SnipWiz/**.*' }
		UsePlugin()
		wxCrafterTarget('SnipWiz', 'templateclassbasedlg.wxcp', {'templateclassbasedlg.cpp', 'templateclassbasedlg.hpp', 'templateclassbasedlg_snipwiz_bitmaps.cpp'})
		filter { 'files:SnipWiz/snipwiz.rc' }
			buildaction 'None'
		filter {}
-- ----------------------------------------------------------------------------
	project 'SpellChecker'
		kind 'SharedLib'
		files { 'SpellChecker/**.*' }
		includedirs { 'SpellChecker' }
		UseHunspell()
		UsePlugin()
		wxCrafterTarget('SpellChecker', 'wxcrafter.wxcp', {'wxcrafter.cpp', 'wxcrafter.hpp', 'wxcrafter_bitmaps.cpp'})
		filter 'toolset:msc*'
			disablewarnings {
				'4244', -- 'argument': conversion from '$type1' to '$type2', possible loss of data
				'4267', -- '$op': conversion from '$type1' to '$type2', possible loss of data
				'4273', -- '$var': inconsistent dll linkage
				'4706', -- assignment within conditional expression
				'4996', -- DEPRECATIONs
			}
		filter { 'files:SpellChecker/hunspell/*.cxx' } -- TODO REMOVE
			buildaction 'None' -- That file is included
		filter {}
-- ----------------------------------------------------------------------------
	project 'Subversion2'
		kind 'SharedLib'
		files { 'Subversion2/**.*' }
		UsePlugin()
		wxCrafterTarget('Subversion2', 'subversion2.wxcp', {'subversion2_ui.cpp', 'subversion2_ui.hpp', 'subversion2_subversion2_bitmaps.cpp'})
		wxCrafterTarget('Subversion2', 'wxcrafter.wxcp', {'wxcrafter.cpp', 'wxcrafter.hpp', 'wxcrafter_subversion2_bitmaps.cpp'})

		filter 'toolset:msc*'
			disablewarnings {
				'4457', -- declaration of '$var' hides function parameter
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'Tail'
		kind 'SharedLib'
		files { 'Tail/**.*' }
		UsePlugin()
		wxCrafterTarget('Tail', 'TailUI.wxcp', {'TailUI.cpp', 'TailUI.hpp', 'TailUI_tail_bitmaps.cpp'})

-- ----------------------------------------------------------------------------
	project 'UnitTestCPP'
		kind 'SharedLib'
		files { 'UnitTestCPP/**.*' }
		UsePlugin()
		wxCrafterTarget('UnitTestCPP', 'unittestreport.wxcp', {'unittestreport.cpp', 'unittestreport.hpp', 'unittestreport_unittestcpp_bitmaps.cpp'})
		wxCrafterTarget('UnitTestCPP', 'NewClassTestBase.wxcp', {'testclassbasedlg.cpp', 'testclassbasedlg.hpp', 'NewClassTestBase_unittestcpp_bitmaps.cpp'})

		filter 'toolset:msc*'
			disablewarnings {
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'WordCompletion'
		kind 'SharedLib'
		files { 'WordCompletion/**.*' }
		includedirs { 'WordCompletion' }
		defines {'__WIN32'} -- to fix <unitstd.h>
		UsePlugin()

		wxCrafterTarget('WordCompletion', 'UI.wxcp', {'UI.cpp', 'UI.hpp', 'UI_wordcompletion_bitmaps.cpp'})

		filter 'files:WordCompletion/WordTokenizer.l'
			flex_command('word')
		filter 'toolset:msc*'
			disablewarnings {
				'4189', -- '%var': local variable is initialized but not referenced
				'4273', -- '%func': inconsistent dll linkage
				'4390', -- ';': empty controlled statement found; is this the intent?
				'4505', -- '%func': unreferenced function with internal linkage has been removed
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'wxformbuilder'
		kind 'SharedLib'
		files { 'wxformbuilder/**.*' }
		UsePlugin()
-- ----------------------------------------------------------------------------
	project 'ZoomNavigator'
		kind 'SharedLib'
		files { 'ZoomNavigator/**.*' }
		UsePlugin()
		wxCrafterTarget('ZoomNavigator', 'zoom_navigator.wxcp', { 'zoom_navigator.cpp', 'zoom_navigator.hpp', 'zoom_navigator_zoomnavigator_bitmaps.cpp' })
-- ----------------------------------------------------------------------------
group 'applications_standalone'
-- ----------------------------------------------------------------------------
	project 'codelite_echo'
		kind 'ConsoleApp'
		files { 'codelite_echo/**.*' }
-- ----------------------------------------------------------------------------
	project 'le_exec'
		kind 'ConsoleApp'
		undefines { 'UNICODE' }
		files { 'le_exec/**.*' }
		filter 'toolset:msc*'
			disablewarnings {
				'4267', -- '+=': conversion from '%type' to '%type2', possible loss of data
				'4996', -- '$func_or_var': This function or variable may be is unsafe. Consider using $func_or_var2 instead. To disble deprecation use _CRT_SECURE_NO_WARNINGS.
			}
		filter {}

-- ----------------------------------------------------------------------------
	project 'codelite_makedir' -- makedir
		kind 'ConsoleApp'
		files { 'codelite_makedir/**.*', 'TestDir/**.*' }
		targetname 'makedir'
-- ----------------------------------------------------------------------------
group 'library/CxxParser'
-- ----------------------------------------------------------------------------
	project 'CxxParser'
		kind 'ConsoleApp'
		includedirs('CxxParser')
		files { 'CxxParser/main.cpp' }
		links { 'LibCxxParser' }
		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '%var': unreferenced formal parameter
				'4267', -- 'initializing': conversion from 'size_t' to 'long', possible loss of data
				'4996', -- DEPRECATIONs
			}
		filter {}

-- ----------------------------------------------------------------------------
	project 'LibCxxParser'
		kind 'StaticLib'
		includedirs('CxxParser')
		files { 'CxxParser/**.*' }
		removefiles { 'CxxParser/main.cpp' }

		filter 'files:CxxParser/cpp.l'
			flex_command('cl_scope_')
		filter 'files:CxxParser/expr_lexer.l'
			flex_command('cl_expr_')

		filter 'files:CxxParser/typedef_grammar.y'
			yacc_command('cl_typedef_')
		filter 'files:CxxParser/cpp_variables_grammar.y'
			yacc_command('cl_var_')
		filter 'files:CxxParser/expr_grammar.y'
			yacc_command('cl_expr_')
		filter 'files:CxxParser/cpp_func_parser.y'
			yacc_command('cl_func_')
		filter 'files:CxxParser/cpp_scope_grammar.y'
			yacc_command('cl_scope_', 'cpp_lexer.h')

		filter 'toolset:msc*'
			disablewarnings {
				'4100', -- '$var': unreferenced formal parameter
				'4102', -- '$label': unreferenced label
				'4244', -- '%op': conversion from '%type1' to '%type2', possible loss of data
				'4267', -- '%op': conversion from '%type1' to '%type2', possible loss of data
				'4477', --'printf': format string '%d' requires an argument of type 'int', but variadic argument 1 has type 'int64'
				'4505', -- '%func': unreferenced function with internal linkage has been removed
				'4706', -- assignment within conditional expression
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'CxxParserTests'
		kind 'ConsoleApp'
		files { 'CxxParserTests/**.*' }
		UseLibCodelite()
		UseDocTest()
		links {'LibCxxParser'}

-- ----------------------------------------------------------------------------
group 'application_lib'
-- ----------------------------------------------------------------------------
	project 'codelite_make'
		kind 'ConsoleApp'

		files { 'codelite_make/**.*' }
		UsePlugin()
-- ----------------------------------------------------------------------------
	project 'codelite-generate-themes'
		kind 'WindowedApp'
		files { 'codelite-generate-themes/**.*' }
		UsePlugin()
		includedirs { 'Plugin/ThemeImporters' }
		defines { 'wxDEBUG_LEVEL=0' }
		wxCrafterTarget('codelite-generate-themes/codelite-generate-themes', 'wxcrafter.wxcp', {'wxcrafter.cpp', 'wxcrafter.hpp', 'wxcrafter_codelite-generate-themes_bitmaps.cpp'})
		filter 'files:codelite-generate-themes/**.rc'
			buildaction 'None'
		filter {}
-- ----------------------------------------------------------------------------
group 'application_lib'
-- ----------------------------------------------------------------------------
	project 'LiteEditor'
		kind 'WindowedApp'
		files { 'LiteEditor/**.*' }
		includedirs { 'Plugin/ThemeImporters' }
		includedirs { 'solution/%{_ACTION}' } -- for generated files (autoversion.h)
		includedirs { 'CxxParser' }
		defines { '_HITLOGGING_DEFINED=0' } -- mingw
		UsePlugin()
		-- wxCrafterTarget('LiteEditor', 'AboutDlg.wxcp', {'.cpp', '.hpp', 'AboutDlg_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'AccelTableBaseDlg.wxcp', {'AccelTableBaseDlg.cpp', 'AccelTableBaseDlg.hpp', 'AccelTableBaseDlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'AddOptionsDialogBase.wxcp', {'AddOptionsDialogBase.cpp', 'AddOptionsDialogBase.hpp', 'AddOptionsDialogBase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'advance_settings.wxcp', {'advance_settings.cpp', 'advance_settings.hpp', 'advance_settings_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'AttachDbgProcBaseDlg.wxcp', {'attachdbgprocbasedlg.cpp', 'attachdbgprocbasedlg.hpp', 'AttachDbgProcBaseDlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'breakpointdlg.wxcp', {'breakpointdlgbase.cpp', 'breakpointdlgbase.hpp', 'breakpoint_dlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'BuildTabSettings.wxcp', {'buildsettingstabbase.cpp', 'buildsettingstabbase.hpp', 'buildsettingstab_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'compiler_page.wxcp', {'compiler_pages.cpp', 'compiler_pages.hpp', 'compiler_page_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'CompilersFoundDlg.wxcp', {'CompilersFoundDlgBase.cpp', 'CompilersFoundDlgBase.hpp', 'CompilersFoundDlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'ConfigManagerBaseDlg.wxcp', {'ConfigManagerBaseDlg.cpp', 'ConfigManagerBaseDlg.hpp', 'ConfigManagerBaseDlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'DbgCommandBaseDlg.wxcp', {'DbgCommandBaseDlg.cpp', 'DbgCommandBaseDlg.hpp', 'DbgCommandBaseDlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'DebuggerSettings.wxcp', {'debuggersettingsbasedlg.cpp', 'debuggersettingsbasedlg.hpp', 'DebuggerSettings_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'depend_dlg_page.wxcp', {'dialogspagebase.cpp', 'dialogspagebase.hpp', 'depend_dlg_page_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'editor_options_bookmarks.wxcp', {'.cpp', '.hpp', 'editor_options_bookmarks_liteeditor_bitmaps.cpp'}) --
		-- wxCrafterTarget('LiteEditor', 'editor_options_caret.wxcp', {'.cpp', '.hpp', 'editor_options_caret_liteeditor_bitmaps.cpp'}) --
		-- wxCrafterTarget('LiteEditor', 'editor_options_comments_doxygen.wxcp', {'.cpp', '.hpp', '_liteeditor_bitmaps.cpp'}) --
		-- wxCrafterTarget('LiteEditor', 'editor_options_docking_windows.wxcp', {'.cpp', '.hpp', 'editor_options_docking_windows_liteeditor_bitmaps.cpp'}) --
		-- wxCrafterTarget('LiteEditor', 'editor_options_guides.wxcp', {'.cpp', '.hpp', '_liteeditor_bitmaps.cpp'}) --
		--wxCrafterTarget('LiteEditor', '.wxcp', {'.cpp', '.hpp', 'editorsettingslocalbase_formbuilder_bitmaps.cpp'}) --
		wxCrafterTarget('LiteEditor', 'filechecklistbase.wxcp', {'filechecklistbase.cpp', 'filechecklistbase.hpp', 'filechecklistbase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'importfilesdialog.wxcp', {'importfilesdialog_new.cpp', 'importfilesdialog_new.hpp', 'importfilesdialog_liteeditor_bitmaps.cpp'}) --
		wxCrafterTarget('LiteEditor', 'movefuncimplbasedlg.wxcp', {'movefuncimplbasedlg.cpp', 'movefuncimplbasedlg.hpp', 'movefuncimplbasedlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'new_virtual_folder.wxcp', {'new_virtual_folder.cpp', 'new_virtual_folder.hpp', 'new_virtual_folder_liteeditor_bitmaps.cpp'})
		-- wxCrafterTarget('LiteEditor', 'new_workspace_dlg.wxcp', {'.cpp', '.hpp', 'new_workspace_dlg_liteeditor_bitmaps.cpp'}) --
		wxCrafterTarget('LiteEditor', 'newquickwatch.wxcp', {'newquickwatch.cpp', 'newquickwatch.hpp', 'newquickwatch_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'OpenWindowsPanelBase.wxcp', {'openwindowspanelbase.cpp', 'openwindowspanelbase.hpp', 'OpenWindowsPanelBase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'optionsdialogbase2.wxcp', {'options_base_dlg2.cpp', 'options_base_dlg2.hpp', 'optionsdialogbase2_liteeditor_bitmaps.cpp'}) -- fbp
		wxCrafterTarget('LiteEditor', 'plugindlgbase.wxcp', {'plugindlgbase.cpp', 'plugindlgbase.hpp', 'plugindlgbase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'project_settings.wxcp', {'project_settings_base_dlg.cpp', 'project_settings_base_dlg.hpp', 'project_settings_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'quickfindbarbase.wxcp', {'quickfindbarbase.cpp', 'quickfindbarbase.hpp', 'quickfindbarbase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'reconcileproject.wxcp', {'reconcileproject.cpp', 'reconcileproject.hpp', 'reconcileproject_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'SyntaxHighlightBaseDlg.wxcp', {'syntaxhighlightbasedlg.cpp', 'syntaxhighlightbasedlg.hpp', 'syntaxhighlightbasedlg_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'tabgroupbasedlgs.wxcp', {'tabgroupbasedlgs.cpp', 'tabgroupbasedlgs.hpp', 'tabgroupbasedlgs_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'ThreadListPanelBase.wxcp', {'ThreadListPanelBase.cpp', 'ThreadListPanelBase.hpp', 'ThreadListPanelBase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'workspacesettingsbase.wxcp', {'workspacesettingsbase.cpp', 'workspacesettingsbase.hpp', 'workspacesettingsbase_liteeditor_bitmaps.cpp'})
		wxCrafterTarget('LiteEditor', 'wxcrafter.wxcp', {'wxcrafter.cpp', 'wxcrafter.hpp', 'wxcrafter_liteeditor_bitmaps.cpp'})

		filter 'files:LiteEditor/**.rc'
			buildaction 'None'
		filter 'toolset:msc*'
			disablewarnings {
				'4189', -- '$var': local variable is initialized but not referenced
				'4245', -- 'initializing': conversion from '%type1' to '%type2', signed/unsigned mismatch
				'4456', -- declaration of '$var' hides previous local declaration
				'4457', -- declaration of '$var' hides function parameter
				'4996', -- DEPRECATIONs
			}
		filter {}
-- ----------------------------------------------------------------------------
	project 'CodeliteTest'
		kind 'ConsoleApp'
		files { 'Tests/CodeliteTest/**' }
        removefiles { 'Tests/CodeliteTest/MarkdownStylerTests.cpp', 'Tests/CodeliteTest/test_patch_applier.cpp' } -- should go in PluginTest
		UseDocTest()
		UseLibCodelite()
-- ----------------------------------------------------------------------------
	project 'wxcrafter'
		kind 'WindowedApp' -- or 'SharedLib' as Plugins
		files { 'wxcrafter/**.*' }
		files { 'solution/%{_ACTION}/wxcversion.cpp'}
		includedirs { 'wxcrafter', 'wxcrafter/aui', 'wxcrafter/controls', 'wxcrafter/sizers', 'wxcrafter/src', 'wxcrafter/top_level_windows', 'wxcrafter/xrc_handlers' }
		UsePlugin()

		prebuildcommands
		{
			'{COPYFILEIFNEWER} %[wxcrafter/wxgui.zip] %[%{cfg.buildtarget.directory}/../../]',
			'{COPYDIR} %[Runtime/lexers] %[%{cfg.buildtarget.directory}/../../lexers/]',
			'{COPYDIR} %[svgs] %[%{cfg.buildtarget.directory}/../../svgs/]',
		}

		filter { 'kind:WindowedApp' }
			defines { 'STANDALONE_BUILD=1' }
		filter { 'kind:SharedLib' }
			defines { 'WXCAPP=0' }
		filter 'files:wxcrafter/resources/**.*'
			buildaction 'None'

		filter 'toolset:msc*'
			disablewarnings {
				'4244', -- '=': conversion from 'unsigned int' to 'char', possible loss of data
				'4267', -- '+=': conversion from 'size_t' to 'int', possible loss of data
				'4334', -- '<<': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
				'4456', -- declaration of '$var' hides previous local declaration
				'4457', -- declaration of '$var' hides function parameter
				'4706', -- assignment within conditional expression
				'4996', -- DEPRECATIONs
			}
		filter {}
