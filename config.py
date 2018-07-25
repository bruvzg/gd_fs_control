def can_build(env, platform):
    if platform in ['server', 'javascript', 'ios', 'android']:
        return False
    return True

def configure(env):
    pass

def get_doc_classes():
    return [
        "FSController",
    ]

def get_doc_path():
    return "doc_classes"
