import functools as functools________________________________long_name


def main________________________________long_name():
	try:
		def hook(name):
			func = globals().get(name)
			if func is not None and callable(func):

				def decorator(hook_func):
					@functools________________________________long_name.wraps(func)
					def wrapper(*args, **kwargs):
						return hook_func(func, *args, **kwargs)

					globals()[name] = wrapper

				return decorator
		# define here your functions
		@hook("foo")
		def foo_hook(original_func, *args, **kwargs):
			print("hook2", globals())
			return original_func(*args, **kwargs)
	except:
		pass


main________________________________long_name()
del (
	functools________________________________long_name,
	main________________________________long_name,
)
