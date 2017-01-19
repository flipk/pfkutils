function fish_prompt --description 'Write out the prompt'

	set -l last_status $status
	set -l color_cwd
	set -l suffix

	switch $USER
	case root toor
		if set -q fish_color_cwd_root
			set color_cwd $fish_color_cwd_root
		else
			set color_cwd $fish_color_cwd
		end
		set suffix '#'
	case '*'
		set color_cwd $fish_color_cwd
		set suffix '%'
	end

	if not set -q __fish_prompt_normal
		set -g __fish_prompt_normal (set_color normal)
	end
	if not set -q __fish_prompt_color_cwd
		set -g __fish_prompt_color_cwd (set_color $color_cwd)
	end
	if not set -q __fish_prompt_color_user
		set -g __fish_prompt_color_user (set_color $fish_color_user)
	end
	if not set -q __fish_prompt_color_suffix
		set -g __fish_prompt_color_suffix (set_color black --background green)
	end

	printf \
		"%s%s%s\n%s%s@%s%s pid %s on %s status %s fish %s\n %s%s%s " \
		$__fish_prompt_color_cwd $PWD $__fish_prompt_normal \
		$__fish_prompt_color_user $USER $HOST $__fish_prompt_normal \
		%self $pfk_tty $last_status $version \
		$__fish_prompt_color_suffix $suffix $__fish_prompt_normal

end
