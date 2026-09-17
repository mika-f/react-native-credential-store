require 'json'
package = JSON.parse(File.read(File.join(__dir__, 'package.json')))

Pod::Spec.new do |s|
  s.name = 'RNSecureStore'
  s.version = package['version']
  s.summary = package['description']
  s.homepage = 'https://github.com/mika-f/react-native-credential-store'
  s.license = { :type => 'MIT', :file => 'LICENSE' }
  s.author = 'Natsuneko'
  s.source = { :git => 'https://github.com/mika-f/react-native-credential-store.git', :tag => "v#{s.version}" }
  s.platforms = { :osx => '14.0' }
  s.source_files = 'macos/*.{h,mm}'
  s.frameworks = 'Security', 'Foundation'
  s.requires_arc = true
  install_modules_dependencies(s)
end
