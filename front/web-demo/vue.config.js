module.exports = {
  devServer: {
    proxy: {
      '/api': {
        target: 'http://mitsusplit.local:80',
        changeOrigin: true,
        ws: true
      }
    }
  }
}
