# libavproto - avim av层协议的实现

---

使用办法, 需要将 libavim 和 avim_proto_doc 两个仓库同时作为 submodule 添加进来.

然后使用

	add_subdirectory(avim_proto_doc)
	add_subdirectory(libavim)

然后用

	add_executable(yourtarget xxx.cpp)
	target_link_libraries(yourtarget avim++)

就可以使用了.

如果使用 C 接口, 则使用

	target_link_libraries(yourtarget avproto)

# 修改了 submodule 怎么提交?

在 submodule 里使用

	git remote set-url origin  --push git@github.com:avplayer/libavim.git

来设定 ssh 协议, 这样就能 push 了. 然后正常的 commit + push 就可以了.

注意通常如果是 父项目 git submodule update , 那么子项目里的 git 状态是没有处于任何分支.
在开始修改前请 确保 git checkout master

