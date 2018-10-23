module("luci.controller.auth.devauth", package.seeall)

function index()
	entry({"admin", "system", "devauth"}, cbi("auth/devauth"), _("授权管理"), 89).dependent = true
end

