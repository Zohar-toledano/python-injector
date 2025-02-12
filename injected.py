import functools


def hook(func,name):
	globals()[name] = func(globals()[name])

# Define the decorator
def hook1(func):
    @functools.wraps(func)  # Ensures the original function's metadata is preserved
    def wrapper(*args, **kwargs):
        print(f"hooked function: {func.__name__}")
        return func(*args, **kwargs)
    return wrapper

hook(hook1,"foo")