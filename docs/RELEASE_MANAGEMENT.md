# SystemMonitor CI/CD & Release Management Guide

This document describes the complete CI/CD pipeline and release process for SystemMonitor.

## 🚀 Automated CI/CD Pipeline

### Continuous Integration (Every Commit)
✅ **Automatic build** on every push to `main` or `develop`
✅ **Automatic build** on every pull request 
✅ **Build status badges** in README.md
✅ **Caching** for faster builds (vcpkg dependencies)
✅ **Build artifacts** stored for 30 days
✅ **PR comments** with build status

### Continuous Deployment (Tagged Releases)
✅ **Automatic release** when you push a git tag `v*`
✅ **Release package** generation with deployment docs
✅ **GitHub release** creation with changelogs
✅ **ZIP archives** ready for distribution

## 🔄 Developer Workflow

### For Regular Development:
```bash
# 1. Make your changes
git add .
git commit -m "feat: Add new monitoring feature"

# 2. Push to GitHub - CI automatically builds and tests
git push origin main

# 3. Check build status at: https://github.com/dnguyenminh/SystemMonitor/actions
```

### For Releases:
```bash
# 1. Prepare release locally (optional - for testing)
release.bat prepare v1.1.0

# 2. Create and push tag - GitHub automatically creates release
git tag v1.1.0
git push origin v1.1.0

# 3. GitHub Actions will:
#    - Build the project
#    - Create release package  
#    - Publish GitHub release with ZIP
```

## 📊 Build Status Monitoring

### Build Badges in README:
- **CI/CD Status**: Shows if latest build passed/failed
- **Build Status**: Detailed build information
- **Latest Release**: Current release version

### Where to Check Build Status:
1. **GitHub Actions Tab**: https://github.com/dnguyenminh/SystemMonitor/actions
2. **README Badges**: Green = passing, Red = failing
3. **PR Comments**: Automatic build summaries on pull requests

## 📦 Release Package Contents

Each release contains:
```
SystemMonitor-v1.0.0/
├── SystemMonitor.exe              # Main executable (static linked)
├── config/
│   └── SystemMonitor.cfg.template # Configuration template
├── docs/                          # Documentation
├── README.md                      # Project readme
└── DEPLOYMENT_README.md           # Deployment guide
```

## 🔧 Configuration Management

### For Development:
- Use `config/SystemMonitor.cfg` (ignored by git)
- Contains your actual email credentials

### For Releases:
- Use `config/SystemMonitor.cfg.template` (tracked by git)
- Contains example configuration without sensitive data
- Users copy and customize for their environment

## 🌟 Best Practices

### Version Numbering:
- Use semantic versioning: `v1.0.0`
- Major.Minor.Patch format
- Tag format: `vX.Y.Z`

### Commit Messages:
```bash
# Good commit messages
git commit -m "feat: Add TLS email notification support"
git commit -m "fix: Resolve mutex deadlock in system monitoring"
git commit -m "docs: Update deployment instructions"

# Release commits
git commit -m "Release v1.0.0: Production build with static linking"
```

### Branch Strategy:
- `main` - Stable, production-ready code
- `develop` - Integration branch for features
- `feature/*` - Individual feature branches
- `hotfix/*` - Critical fixes for releases

## 🔄 Complete Workflow Example

```bash
# 1. Prepare your code
git add .
git commit -m "feat: Complete SystemMonitor with TLS email alerts"

# 2. Create release
release.bat prepare v1.0.0

# 3. Push to GitHub
release.bat push v1.0.0

# 4. GitHub Actions automatically:
#    - Builds the project
#    - Creates release package
#    - Publishes GitHub release
```

## 🚨 Troubleshooting

### Build Fails:
```bash
release.bat clean    # Clean artifacts
release.bat build    # Test build
```

### Git Issues:
```bash
release.bat status   # Check repository state
git log --oneline    # Review recent commits
```

### Release Package Issues:
- Check `releases/` directory for generated files
- Verify ZIP archive contents
- Test deployment package on clean machine

## 📊 Release Checklist

Before creating a release:
- [ ] All tests pass
- [ ] Documentation updated
- [ ] Configuration template updated
- [ ] Version numbers updated
- [ ] CHANGELOG.md updated (if applicable)
- [ ] No sensitive data in repository

After creating a release:
- [ ] GitHub release published
- [ ] Release notes complete
- [ ] Deployment tested
- [ ] Documentation links working
- [ ] Archive downloadable

## 🔗 Related Files

- `release.bat` - Main release management script
- `scripts/prepare-release.bat` - Detailed release preparation
- `.github/workflows/build-release.yml` - CI/CD automation
- `config/SystemMonitor.cfg.template` - Configuration template
- `.gitignore` - Git ignore rules for releases
